################################################################################
#                                                                              #
#                This file is part of the Buildbotics firmware.                #
#                                                                              #
#                  Copyright (c) 2015 - 2018, Buildbotics LLC                  #
#                             All rights reserved.                             #
#                                                                              #
#     This file ("the software") is free software: you can redistribute it     #
#     and/or modify it under the terms of the GNU General Public License,      #
#      version 2 as published by the Free Software Foundation. You should      #
#      have received a copy of the GNU General Public License, version 2       #
#     along with the software. If not, see <http://www.gnu.org/licenses/>.     #
#                                                                              #
#     The software is distributed in the hope that it will be useful, but      #
#          WITHOUT ANY WARRANTY; without even the implied warranty of          #
#      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU       #
#               Lesser General Public License for more details.                #
#                                                                              #
#       You should have received a copy of the GNU Lesser General Public       #
#                License along with the software.  If not, see                 #
#                       <http://www.gnu.org/licenses/>.                        #
#                                                                              #
#                For information regarding this software email:                #
#                  "Joseph Coffland" <joseph@buildbotics.com>                  #
#                                                                              #
################################################################################

import os
import time
import json
import hashlib
import glob
import tempfile
import signal
from concurrent.futures import Future
from tornado import gen, process, iostream
import bbctrl


def hash_dump(o):
    s = json.dumps(o, separators = (',', ':'), sort_keys = True)
    return s.encode('utf8')


def plan_hash(path, config):
    h = hashlib.sha256()
    h.update('v4'.encode('utf8'))
    h.update(hash_dump(config))

    with open(path, 'rb') as f:
        while True:
            buf = f.read(1024 * 1024)
            if not buf: break
            h.update(buf)

    return h.hexdigest()


def safe_remove(path):
    try:
        os.unlink(path)
    except: pass


class Plan(object):
    def __init__(self, preplanner, ctrl, filename):
        self.preplanner = preplanner

        # Copy planner state
        self.state = ctrl.state.snapshot()
        self.config = ctrl.mach.planner.get_config(False, False)
        del self.config['default-units']

        self.progress = 0
        self.cancel = False
        self.pid = None

        root = ctrl.get_path()
        self.gcode = '%s/upload/%s' % (root, filename)
        self.base = '%s/plans/%s' % (root, filename)
        self.hid = plan_hash(self.gcode, self.config)
        fbase = '%s.%s.' % (self.base, self.hid)
        self.files = [
            fbase + 'json',
            fbase + 'positions.gz',
            fbase + 'speeds.gz']

        self.future = Future()
        ctrl.ioloop.add_callback(self._load)


    def terminate(self):
        if self.cancel: return
        self.cancel = True
        if self.pid is not None:
            try:
                os.kill(self.pid, signal.SIGKILL)
            except: pass


    def delete(self):
        files = glob.glob(self.base + '.*')
        for path in files: safe_remove(path)


    def clean(self, max = 2):
        plans = glob.glob(self.base + '.*.json')
        if len(plans) <= max: return

        # Delete oldest plans
        plans = [(os.path.getmtime(path), path) for path in plans]
        plans.sort()

        for mtime, path in plans[:len(plans) - max]:
            safe_remove(path)
            safe_remove(path[:-4] + 'positions.gz')
            safe_remove(path[:-4] + 'speeds.gz')


    def _exists(self):
        for path in self.files:
            if not os.path.exists(path): return False
        return True


    def _read(self):
        if self.cancel: return

        try:
            with open(self.files[0], 'r')  as f: meta = json.load(f)
            with open(self.files[1], 'rb') as f: positions = f.read()
            with open(self.files[2], 'rb') as f: speeds = f.read()

            return meta, positions, speeds

        except:
            self.preplanner.log.exception()

            # Clean
            for path in self.files:
                if os.path.exists(path):
                    os.remove(path)


    @gen.coroutine
    def _exec(self):
        self.clean() # Clean up old plans

        with tempfile.TemporaryDirectory() as tmpdir:
            cmd = (
                '/usr/bin/env', 'python3',
                bbctrl.get_resource('plan.py'),
                os.path.abspath(self.gcode), json.dumps(self.state),
                json.dumps(self.config),
                '--max-time=%s' % self.preplanner.max_plan_time,
                '--max-loop=%s' % self.preplanner.max_loop_time
            )

            self.preplanner.log.info('Running: %s', cmd)

            proc = process.Subprocess(cmd, stdout = process.Subprocess.STREAM,
                                      stderr = process.Subprocess.STREAM,
                                      cwd = tmpdir)
            errs = ''
            self.pid = proc.proc.pid

            try:
                try:
                    while True:
                        line = yield proc.stdout.read_until(b'\n')
                        self.progress = float(line.strip())
                        if self.cancel: return
                except iostream.StreamClosedError: pass

                self.progress = 1

                ret = yield proc.wait_for_exit(False)
                if ret:
                    errs = yield proc.stderr.read_until_close()
                    raise Exception('Plan failed: ' + errs.decode('utf8'))

            finally:
                proc.stderr.close()
                proc.stdout.close()

            if not self.cancel:
                os.rename(tmpdir + '/meta.json',    self.files[0])
                os.rename(tmpdir + '/positions.gz', self.files[1])
                os.rename(tmpdir + '/speeds.gz',    self.files[2])
                os.sync()


    @gen.coroutine
    def _load(self):
        try:
            if self._exists():
                data = self._read()
                if data is not None:
                    self.future.set_result(data)
                    return

            if not self._exists(): yield self._exec()
            self.future.set_result(self._read())

        except:
            self.preplanner.log.exception()



class Preplanner(object):
    def __init__(self, ctrl, max_plan_time = 60 * 60 * 24, max_loop_time = 300):
        self.ctrl = ctrl
        self.log = ctrl.log.get('Preplanner')

        self.max_plan_time = max_plan_time
        self.max_loop_time = max_loop_time

        path = self.ctrl.get_plan()
        if not os.path.exists(path): os.mkdir(path)

        self.started = Future()
        self.plans = {}


    def start(self):
        if not self.started.done():
            self.log.info('Preplanner started')
            self.started.set_result(True)


    def invalidate(self, filename):
        if filename in self.plans:
            self.plans[filename].terminate()
            del self.plans[filename]


    def invalidate_all(self):
        for filename, plan in self.plans.items():
            plan.terminate()
        self.plans = {}


    def delete_all_plans(self):
        files = glob.glob(self.ctrl.get_plan('*'))
        for path in files: safe_remove(path)
        self.invalidate_all()


    def delete_plans(self, filename):
        if filename in self.plans:
            self.plans[filename].delete()
            self.invalidate(filename)

    @gen.coroutine
    def get_plan(self, filename):
        if filename is None: raise Exception('Filename cannot be None')

        # Wait until state is fully initialized
        yield self.started

        if filename in self.plans: plan = self.plans[filename]
        else:
            plan = Plan(self, self.ctrl, filename)
            self.plans[filename] = plan

        data = yield plan.future
        return data


    def get_plan_progress(self, filename):
        return self.plans[filename].progress if filename in self.plans else 0
