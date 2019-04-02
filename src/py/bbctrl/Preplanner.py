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
import threading
import subprocess
import tempfile
from concurrent.futures import Future, ThreadPoolExecutor, TimeoutError
from tornado import gen
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
            time.sleep(0.0001) # Yield some time

    return h.hexdigest()


def safe_remove(path):
    try:
        os.unlink(path)
    except: pass


class Plan(object):
    def __init__(self, preplanner, root, filename):
        self.preplanner = preplanner
        self.progress = 0
        self.cancel = threading.Event()
        self.gcode = '%s/upload/%s' % (root, filename)
        self.base = '%s/plans/%s' % (root, filename)


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


    def _update_progress(self, progress):
        with self.preplanner.lock:
            self.progress = progress


    def _exec(self, files, state, config):
        self.clean() # Clean up old plans

        with tempfile.TemporaryDirectory() as tmpdir:
            cmd = (
                '/usr/bin/env', 'python3',
                bbctrl.get_resource('plan.py'),
                os.path.abspath(self.gcode), json.dumps(state),
                json.dumps(config),
                '--max-time=%s' % self.preplanner.max_plan_time,
                '--max-loop=%s' % self.preplanner.max_loop_time
            )

            self.preplanner.log.info('Running: %s', cmd)

            with subprocess.Popen(cmd, stdout = subprocess.PIPE,
                                  stderr = subprocess.PIPE,
                                  cwd = tmpdir) as proc:

                for line in proc.stdout:
                    self._update_progress(float(line))
                    if self.cancel.is_set():
                        proc.terminate()
                        return

                out, errs = proc.communicate()

                self._update_progress(1)
                if self.cancel.is_set(): return

                if proc.returncode:
                    raise Exception('Plan failed: ' + errs.decode('utf8'))

            os.rename(tmpdir + '/meta.json', files[0])
            os.rename(tmpdir + '/positions.gz', files[1])
            os.rename(tmpdir + '/speeds.gz', files[2])
            os.sync()


    def load(self, state, config):
        try:
            os.nice(5)

            hid = plan_hash(self.gcode, config)
            base = '%s.%s.' % (self.base, hid)
            files = [base + 'json', base + 'positions.gz', base + 'speeds.gz']

            def exists():
                for path in files:
                    if not os.path.exists(path): return False
                return True

            def read():
                if self.cancel.is_set(): return

                try:
                    with open(files[0], 'r')  as f: meta = json.load(f)
                    with open(files[1], 'rb') as f: positions = f.read()
                    with open(files[2], 'rb') as f: speeds = f.read()

                    return meta, positions, speeds

                except:
                    self.preplanner.log.exception()

                    for path in files:
                        if os.path.exists(path):
                            os.remove(path)

            if exists():
                data = read()
                if data is not None: return data

            if not exists(): self._exec(files, state, config)
            return read()

        except:
            self.preplanner.log.exception()


class Preplanner(object):
    def __init__(self, ctrl, threads = 4, max_plan_time = 60 * 60 * 24,
                 max_loop_time = 300):
        self.ctrl = ctrl
        self.log = ctrl.log.get('Preplanner')

        self.max_plan_time = max_plan_time
        self.max_loop_time = max_loop_time

        path = self.ctrl.get_plan()
        if not os.path.exists(path): os.mkdir(path)

        self.started = Future()

        self.plans = {}
        self.pool = ThreadPoolExecutor(threads)
        self.lock = threading.Lock()


    def start(self):
        if not self.started.done():
            self.log.info('Preplanner started')
            self.started.set_result(True)


    def invalidate(self, filename):
        with self.lock:
            if filename in self.plans:
                self.plans[filename].cancel.set()
                del self.plans[filename]


    def invalidate_all(self):
        with self.lock:
            for filename, plan in self.plans.items():
                plan.cancel.set()
            self.plans = {}


    def delete_all_plans(self):
        files = glob.glob(self.ctrl.get_plan('*'))
        for path in files: safe_remove(path)
        self.invalidate_all()


    def delete_plans(self, filename):
        with self.lock:
            if filename in self.plans:
                self.plans[filename].delete()
                self.invalidate(filename)


    def get_plan(self, filename):
        if filename is None: raise Exception('Filename cannot be None')

        with self.lock:
            if filename in self.plans: plan = self.plans[filename]
            else:
                plan = Plan(self, self.ctrl.get_path(), filename)
                plan.future = self._plan(plan)
                self.plans[filename] = plan

            return plan.future


    def get_plan_progress(self, filename):
        with self.lock:
            if filename in self.plans:
                return self.plans[filename].progress
            return 0


    @gen.coroutine
    def _plan(self, plan):
        # Wait until state is fully initialized
        yield self.started

        # Copy state for thread
        state = self.ctrl.state.snapshot()
        config = self.ctrl.mach.planner.get_config(False, False)
        del config['default-units']

        # Start planner thread
        future = yield self.pool.submit(plan.load, state, config)

        return future
