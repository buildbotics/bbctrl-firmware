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
import gzip
import glob
import threading
from concurrent.futures import Future, ThreadPoolExecutor, TimeoutError
from tornado import gen
import camotics.gplan as gplan # pylint: disable=no-name-in-module,import-error
import bbctrl


log = logging.getLogger('Preplaner')


# Formats floats with no more than two decimal places
def _dump_json(o):
    if isinstance(o, str): yield json.dumps(o)
    elif o is None: yield 'null'
    elif o is True: yield 'true'
    elif o is False: yield 'false'
    elif isinstance(o, int): yield str(o)

    elif isinstance(o, float):
        if o != o: yield 'NaN'
        elif o == float('inf'): yield 'Infinity'
        elif o == float('-inf'): yield '-Infinity'
        else: yield format(o, '.2f')

    elif isinstance(o, (list, tuple)):
        yield '['
        first = True

        for item in o:
            if first: first = False
            else: yield ','
            yield from _dump_json(item)

        yield ']'

    elif isinstance(o, dict):
        yield '{'
        first = True

        for key, value in o.items():
            if first: first = False
            else: yield ','
            yield from _dump_json(key)
            yield ':'
            yield from _dump_json(value)

        yield '}'


def dump_json(o): return ''.join(_dump_json(o))


def hash_dump(o):
    s = json.dumps(o, separators = (',', ':'), sort_keys = True)
    return s.encode('utf8')


def plan_hash(path, config):
    h = hashlib.sha256()
    h.update(hash_dump(config))
    with open('upload/' + path, 'rb') as f: h.update(f.read())
    return h.hexdigest()


class Preplanner(object):
    def __init__(self, ctrl, threads = 4, max_plan_time = 600,
                 max_loop_time = 30):
        self.ctrl = ctrl
        self.max_plan_time = max_plan_time
        self.max_loop_time = max_loop_time

        if not os.path.exists('plans'): os.mkdir('plans')

        self.started = Future()

        self.plans = {}
        self.pool = ThreadPoolExecutor(threads)
        self.lock = threading.Lock()


    def start(self):
        log.info('Preplanner started')
        self.started.set_result(True)


    def invalidate(self, filename):
        with self.lock:
            if filename in self.plans:
                del self.plans[filename]


    def invalidate_all(self):
        with self.lock: self.plans = {}


    def delete_all_plans(self):
        for path in glob.glob('plans/*'):
            try:
                os.unlink(path)
            except OSError: pass

        self.invalidate_all()


    def delete_plans(self, filename):
        for path in glob.glob('plans/' + filename + '.*'):
            try:
                os.unlink(path)
            except OSError: pass

        self.invalidate(filename)


    def get_plan(self, filename):
        with self.lock:
            if filename in self.plans: plan = self.plans[filename]
            else:
                plan = [self._plan(filename), 0]
                self.plans[filename] = plan

            return plan[0]


    def get_plan_progress(self, filename):
        with self.lock:
            if filename in self.plans: return self.plans[filename][1]
            return 0


    @gen.coroutine
    def _plan(self, filename):
        # Wait until state is fully initialized
        yield self.started

        # Copy state for thread
        state = self.ctrl.state.snapshot()
        config = self.ctrl.mach.planner.get_config(False, True)

        # Start planner thread
        plan = yield self.pool.submit(self._exec_plan, filename, state, config)
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
            if not filename in self.plans: return False
            self.plans[filename][1] = progress
            return True


    def _exec_plan(self, filename, state, config):
        # Check if this plan was already run
        hid = plan_hash(filename, config)
        plan_path = 'plans/' + filename + '.' + hid + '.gz'
        if os.path.exists(plan_path):
            with open(plan_path, 'rb') as f: return f.read()

        # Clean up old plans
        self._clean_plans(filename)


        def get_var_cb(name, units):
            value = 0

            if len(name) and name[0] == '_':
                value = state.get(name[1:], 0)
                if units == 'IMPERIAL': value /= 25.4

            return value


        start = time.time()
        moves = []
        line = 0
        totalLines = sum(1 for line in open('upload/' + filename))
        maxLine = 0
        maxLineTime = time.time()
        totalTime = 0
        position = dict(x = 0, y = 0, z = 0)
        rapid = False
        moves = []
        messages = []
        count = 0

        levels = dict(I = 'info', D = 'debug', W = 'warning', E = 'error',
                      C = 'critical')

        def log_cb(level, msg, filename, line, column):
            if level in levels: level = levels[level]
            messages.append(dict(level = level, msg = msg, filename = filename,
                                 line = line, column = column))


        self.ctrl.mach.planner.log_intercept(log_cb)
        planner = gplan.Planner()
        planner.set_resolver(get_var_cb)
        planner.load('upload/' + filename, config)

        try:
            while planner.has_more():
                cmd = planner.next()
                planner.set_active(cmd['id'])
                # Cannot synchronize with actual machine so fake it
                if planner.is_synchronizing(): planner.synchronize(0)

                if cmd['type'] == 'line':
                    totalTime += sum(cmd['times'])

                    target = cmd['target']
                    move = {}

                    for axis in 'xyz':
                        if axis in target:
                            position[axis] = target[axis]
                            move[axis] = target[axis]

                    if 'rapid' in cmd: move['rapid'] = cmd['rapid']

                    moves.append(move)

                elif cmd['type'] == 'set' and cmd['name'] == 'line':
                    line = cmd['value']
                    if maxLine < line:
                        maxLine = line
                        maxLineTime = time.time()

                elif cmd['type'] == 'dwell': totalTime += cmd['seconds']

                if not self._progress(filename, maxLine / totalLines):
                    raise Exception('Plan canceled.')

                if self.max_plan_time < time.time() - start:
                    raise Exception('Max planning time (%d sec) exceeded.' %
                                    self.max_plan_time)

                if self.max_loop_time < time.time() - maxLineTime:
                    raise Exception('Max loop time (%d sec) exceeded.' %
                                    self.max_loop_time)

                count += 1
                if count % 50 == 0: time.sleep(0.001) # Yield some time

        except Exception as e:
            log_cb('error', str(e), filename, line, 0)

        self._progress(filename, 1)

        # Encode data as string
        data = dict(time = totalTime, lines = totalLines, path = moves,
                    messages = messages)
        data = gzip.compress(dump_json(data).encode('utf8'))

        # Save plan
        with open(plan_path, 'wb') as f: f.write(data)

        return data
