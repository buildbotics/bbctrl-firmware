#!/usr/bin/env python3

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

import sys
import argparse
import json
import time
import math
import os
import re
import gzip
import camotics.gplan as gplan # pylint: disable=no-name-in-module,import-error


reLogLine = re.compile(
    r'^(?P<level>[A-Z])[0-9 ]:'
    r'((?P<file>[^:]+):)?'
    r'((?P<line>\d+):)?'
    r'((?P<column>\d+):)?'
    r'(?P<msg>.*)$')



# Formats floats with no more than two decimal places
def _dump_json(o):
    if isinstance(o, str): yield json.dumps(o)
    elif o is None: yield 'null'
    elif o is True: yield 'true'
    elif o is False: yield 'false'
    elif isinstance(o, int): yield str(o)

    elif isinstance(o, float):
        if o != o: yield '"NaN"'
        elif o == float('inf'): yield '"Infinity"'
        elif o == float('-inf'): yield '"-Infinity"'
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


def compute_unit(a, b):
    unit = dict()
    length = 0

    for axis in 'xyzabc':
        if axis in a and axis in b:
            unit[axis] = b[axis] - a[axis]
            length += unit[axis] * unit[axis]

    length = math.sqrt(length)

    for axis in 'xyzabc':
        if axis in unit: unit[axis] /= length

    return unit


def compute_move(start, unit, dist):
    move = dict()

    for axis in 'xyzabc':
        if axis in unit and axis in start:
            move[axis] = start[axis] + unit[axis] * dist

    return move


class Plan(object):
    def __init__(self, path, state, config):
        self.path = path
        self.state = state
        self.config = config

        self.lines = sum(1 for line in open(path))

        self.planner = gplan.Planner()
        self.planner.set_resolver(self.get_var_cb)
        self.planner.set_logger(self._log_cb, 1, 'LinePlanner:3')
        self.planner.load(self.path, config)

        self.messages = []
        self.levels = dict(I = 'info', D = 'debug', W = 'warning', E = 'error',
                           C = 'critical')

        # Initialized axis states and bounds
        self.bounds = dict(min = {}, max = {})
        for axis in 'xyzabc':
            self.bounds['min'][axis] = math.inf
            self.bounds['max'][axis] = -math.inf

        self.maxSpeed = 0
        self.currentSpeed = None
        self.lastProgress = None
        self.lastProgressTime = 0
        self.time = 0


    def add_to_bounds(self, axis, value):
        if value < self.bounds['min'][axis]: self.bounds['min'][axis] = value
        if self.bounds['max'][axis] < value: self.bounds['max'][axis] = value


    def get_bounds(self):
        # Remove infinity from bounds
        for axis in 'xyzabc':
            if self.bounds['min'][axis] == math.inf:
                del self.bounds['min'][axis]
            if self.bounds['max'][axis] == -math.inf:
                del self.bounds['max'][axis]

        return self.bounds


    def update_speed(self, s):
        if self.currentSpeed == s: return False
        self.currentSpeed = s
        if self.maxSpeed < s: self.maxSpeed = s

        return True


    def get_var_cb(self, name, units):
        value = 0

        if len(name) and name[0] == '_':
            value = self.state.get(name[1:], 0)
            if units == 'IMPERIAL': value /= 25.4

        return value


    def log_cb(self, level, msg, filename, line, column):
        if level in self.levels: level = self.levels[level]

        # Ignore missing tool warning
        if (level == 'warning' and
            msg.startswith('Auto-creating missing tool')):
            return

        self.messages.append(
            dict(level = level, msg = msg, filename = filename, line = line,
                 column = column))


    def _log_cb(self, line):
        line = line.strip()
        m = reLogLine.match(line)
        if not m: return

        level = m.group('level')
        msg = m.group('msg')
        filename = m.group('file')
        line = m.group('line')
        column = m.group('column')

        where = ':'.join(filter(None.__ne__, [filename, line, column]))

        if line is not None: line = int(line)
        if column is not None: column = int(column)

        self.log_cb(level, msg, filename, line, column)


    def progress(self, x):
        if time.time() - self.lastProgressTime < 1 and x != 1: return
        self.lastProgressTime = time.time()

        p = '%.4f\n' % x

        if self.lastProgress == p: return
        self.lastProgress = p

        sys.stdout.write(p)
        sys.stdout.flush()


    def _run(self):
        start = time.time()
        line = 0
        maxLine = 0
        maxLineTime = time.time()
        position = {axis: 0 for axis in 'xyzabc'}
        rapid = False

        # Execute plan
        try:
            while self.planner.has_more():
                cmd = self.planner.next()
                self.planner.set_active(cmd['id']) # Release plan

                # Cannot synchronize with actual machine so fake it
                if self.planner.is_synchronizing(): self.planner.synchronize(0)

                if cmd['type'] == 'line':
                    if not (cmd.get('first', False) or
                            cmd.get('seeking', False)):
                        self.time += sum(cmd['times']) / 1000

                    target = cmd['target']
                    move = {}
                    startPos = dict()

                    for axis in 'xyzabc':
                        if axis in target:
                            startPos[axis] = position[axis]
                            position[axis] = target[axis]
                            move[axis] = target[axis]
                            self.add_to_bounds(axis, move[axis])

                    if 'rapid' in cmd: move['rapid'] = cmd['rapid']

                    if 'speeds' in cmd:
                        unit = compute_unit(startPos, target)

                        for d, s in cmd['speeds']:
                            cur = self.currentSpeed

                            if self.update_speed(s):
                                m = compute_move(startPos, unit, d)

                                if cur is not None:
                                    m['s'] = cur
                                    yield m

                                move['s'] = s

                    yield move

                elif cmd['type'] == 'set':
                    if cmd['name'] == 'line':
                        line = cmd['value']
                        if maxLine < line:
                            maxLine = line
                            maxLineTime = time.time()

                    elif cmd['name'] == 'speed':
                        s = cmd['value']
                        if self.update_speed(s): yield {'s': s}

                elif cmd['type'] == 'dwell': self.time += cmd['seconds']

                if args.max_time < time.time() - start:
                    raise Exception('Max planning time (%d sec) exceeded.' %
                                    args.max_time)

                if args.max_loop < time.time() - maxLineTime:
                    raise Exception('Max loop time (%d sec) exceeded.' %
                                    args.max_loop)

                self.progress(maxLine / self.lines)

        except Exception as e:
            self.log_cb('error', str(e), os.path.basename(self.path), line, 0)


    def run(self):
        with gzip.open('plan.json.gz', 'wb') as f:
            def write(s): f.write(s.encode('utf8'))

            write('{"path":[')

            first = True
            for move in self._run():
                if first: first = False
                else: write(',')
                write(dump_json(move))

            write('],')
            write('"time":%.2f,' % self.time)
            write('"lines":%s,' % self.lines)
            write('"maxSpeed":%s,' % self.maxSpeed)
            write('"bounds":%s,' % dump_json(self.get_bounds()))
            write('"messages":%s}' % dump_json(self.messages))


parser = argparse.ArgumentParser(description = 'Buildbotics GCode Planner')
parser.add_argument('gcode', help = 'The GCode file to plan')
parser.add_argument('state', help = 'GCode state variables')
parser.add_argument('config', help = 'Planner config')

parser.add_argument('--max-time', default = 600,
                    type = int, help = 'Maximum planning time in seconds')
parser.add_argument('--max-loop', default = 30,
                    type = int, help = 'Maximum time in loop in seconds')
parser.add_argument('--nice', default = 10,
                    type = int, help = 'Set "nice" process priority')

args = parser.parse_args()

state = json.loads(args.state)
config = json.loads(args.config)

os.nice(args.nice)
plan = Plan(args.gcode, state, config)
plan.run()
