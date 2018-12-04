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
import logging
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


log = logging.getLogger('Preplanner')


def hash_dump(o):
    s = json.dumps(o, separators = (',', ':'), sort_keys = True)
    return s.encode('utf8')


def plan_hash(path, config):
    path = 'upload/' + path
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



class Preplanner(object):
    def __init__(self, ctrl, threads = 4, max_plan_time = 60 * 60 * 24,
                 max_loop_time = 300):
        self.ctrl = ctrl
        self.max_plan_time = max_plan_time
        self.max_loop_time = max_loop_time

        if not os.path.exists('plans'): os.mkdir('plans')

        self.started = Future()

        self.plans = {}
        self.pool = ThreadPoolExecutor(threads)
        self.lock = threading.Lock()


    def start(self):
        if not self.started.done():
            log.info('Preplanner started')
            self.started.set_result(True)


    def invalidate(self, filename):
        with self.lock:
            if filename in self.plans:
                self.plans[filename][2].set() # Cancel
                del self.plans[filename]


    def invalidate_all(self):
        with self.lock:
            for filename, plan in self.plans.items():
                plan[2].set() # Cancel
            self.plans = {}


    def delete_all_plans(self):
        files = glob.glob('plans/*')

        for path in files:
            try:
                os.unlink(path)
            except OSError: pass

        self.invalidate_all()


    def delete_plans(self, filename):
        files = glob.glob('plans/' + filename + '.*')

        for path in files:
            try:
                os.unlink(path)
            except OSError: pass

        self.invalidate(filename)


    def get_plan(self, filename):
        if filename is None: raise Exception('Filename cannot be None')

        with self.lock:
            if filename in self.plans: plan = self.plans[filename]
            else:
                cancel = threading.Event()
                plan = [self._plan(filename, cancel), 0, cancel]
                self.plans[filename] = plan

            return plan[0]


    def get_plan_progress(self, filename):
        with self.lock:
            if filename in self.plans: return self.plans[filename][1]
            return 0


    @gen.coroutine
    def _plan(self, filename, cancel):
        # Wait until state is fully initialized
        yield self.started

        # Copy state for thread
        state = self.ctrl.state.snapshot()
        config = self.ctrl.mach.planner.get_config(False, False, False)
        del config['default-units']

        # Start planner thread
        plan = yield self.pool.submit(
            self._exec_plan, filename, state, config, cancel)
        return plan


    def _clean_plans(self, filename, max = 2):
        plans = glob.glob('plans/' + filename + '.*')
        if len(plans) <= max: return

        # Delete oldest plans
        plans = [(os.path.getmtime(path), path) for path in plans]
        plans.sort()

        for mtime, path in plans[:len(plans) - max]:
            try:
                os.unlink(path)
            except OSError: pass


    def _progress(self, filename, progress):
        with self.lock:
            if filename in self.plans:
                self.plans[filename][1] = progress


    def _exec_plan(self, filename, state, config, cancel):
        try:
            os.nice(5)

            hid = plan_hash(filename, config)
            base = 'plans/' + filename + '.' + hid
            files = [
                base + '.json', base + '.positions.gz', base + '.speeds.gz']

            found = True
            for path in files:
                if not os.path.exists(path): found = False

            if not found:
                self._clean_plans(filename) # Clean up old plans

                path = os.path.abspath('upload/' + filename)
                with tempfile.TemporaryDirectory() as tmpdir:
                    cmd = (
                        '/usr/bin/env', 'python3',
                        bbctrl.get_resource('plan.py'),
                        path, json.dumps(state), json.dumps(config),
                        '--max-time=%s' % self.max_plan_time,
                        '--max-loop=%s' % self.max_loop_time
                    )

                    log.info('Running: %s', cmd)

                    with subprocess.Popen(cmd, stdout = subprocess.PIPE,
                                          stderr = subprocess.PIPE,
                                          cwd = tmpdir) as proc:

                        for line in proc.stdout:
                            self._progress(filename, float(line))
                            if cancel.is_set():
                                proc.terminate()
                                return

                        out, errs = proc.communicate()

                        self._progress(filename, 1)
                        if cancel.is_set(): return

                        if proc.returncode:
                            log.error('Plan failed: ' + errs.decode('utf8'))
                            return # Failed

                    os.rename(tmpdir + '/meta.json', files[0])
                    os.rename(tmpdir + '/positions.gz', files[1])
                    os.rename(tmpdir + '/speeds.gz', files[2])

            with open(files[0], 'r') as f: meta = json.load(f)
            with open(files[1], 'rb') as f: positions = f.read()
            with open(files[2], 'rb') as f: speeds = f.read()

            return meta, positions, speeds

        except Exception as e: log.exception(e)
