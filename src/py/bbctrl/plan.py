#!/usr/bin/env python3

################################################################################
#                                                                              #
#                 This file is part of the Buildbotics firmware.               #
#                                                                              #
#        Copyright (c) 2015 - 2023, Buildbotics LLC, All rights reserved.      #
#                                                                              #
#         This Source describes Open Hardware and is licensed under the        #
#                                 CERN-OHL-S v2.                               #
#                                                                              #
#         You may redistribute and modify this Source and make products        #
#    using it under the terms of the CERN-OHL-S v2 (https:/cern.ch/cern-ohl).  #
#           This Source is distributed WITHOUT ANY EXPRESS OR IMPLIED          #
#    WARRANTY, INCLUDING OF MERCHANTABILITY, SATISFACTORY QUALITY AND FITNESS  #
#     FOR A PARTICULAR PURPOSE. Please see the CERN-OHL-S v2 for applicable    #
#                                  conditions.                                 #
#                                                                              #
#                Source location: https://github.com/buildbotics               #
#                                                                              #
#      As per CERN-OHL-S v2 section 4, should You produce hardware based on    #
#    these sources, You must maintain the Source Location clearly visible on   #
#    the external case of the CNC Controller or other product you make using   #
#                                  this Source.                                #
#                                                                              #
#                For more information, email info@buildbotics.com              #
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
import struct
import math
import bbctrl.camotics as camotics # pylint: disable=no-name-in-module,import-error


reLogLine = re.compile(
    r'^(?P<level>[A-Z])[0-9 ]:'
    r'((?P<file>[^:]+):)?'
    r'((?P<line>\d+):)?'
    r'((?P<column>\d+):)?'
    r'(?P<msg>.*)$')


def clock(): return time.process_time()


def compute_unit(a, b):
    unit = dict()
    length = 0

    for axis in 'xyz':
        if axis in a and axis in b:
            unit[axis] = b[axis] - a[axis]
            length += unit[axis] * unit[axis]

    length = math.sqrt(length)

    if length:
        for axis in 'xyz':
            if axis in unit: unit[axis] /= length

    return unit


def compute_move(start, unit, dist):
    move = dict()

    for axis in 'xyz':
        if axis in unit and axis in start:
            move[axis] = start[axis] + unit[axis] * dist

    return move


class Plan(object):
    def __init__(self, path, state, config):
        self.path = path
        self.state = state
        self.config = config

        self.lines = sum(1 for line in open(path, 'rb'))

        self.planner = camotics.Planner()
        self.planner.set_resolver(self.get_var_cb)
        camotics.set_logger(self._log_cb, 1, 'LinePlanner:3')

        self.messages = []
        self.levels = dict(I = 'info', D = 'debug', W = 'warning', E = 'error',
                           C = 'critical')

        # Initialized axis states and bounds
        self.bounds = dict(min = {}, max = {})
        for axis in 'xyz':
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
        for axis in 'xyz':
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

        msg = dict(
            level    = level,
            msg      = msg,
            filename = filename,
            line     = line,
            column   = column)
        msg = {k: v for k, v in msg.items() if v is not None}

        self.messages.append(msg)


    def _log_cb(self, line):
        line = line.strip()
        m = reLogLine.match(line)
        if not m: return

        level    = m.group('level')
        msg      = m.group('msg')
        filename = m.group('file')
        line     = m.group('line')
        column   = m.group('column')

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
        start = clock()
        line = 0
        maxLine = 0
        maxLineTime = clock()
        position = {axis: 0 for axis in 'xyz'}
        rapid = False

        # Execute plan
        try:
            self.planner.load(self.path, self.config)

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

                    for axis in 'xyz':
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
                            maxLineTime = clock()

                    elif cmd['name'] == 'speed':
                        s = cmd['value']
                        if self.update_speed(s): yield {'s': s}

                elif cmd['type'] == 'dwell': self.time += cmd['seconds']

                if args.max_time < clock() - start:
                    raise Exception('Max planning time (%d sec) exceeded.' %
                                    args.max_time)

                if args.max_loop < clock() - maxLineTime:
                    raise Exception('Max loop time (%d sec) exceeded.' %
                                    args.max_loop)

                if self.lines: self.progress(maxLine / self.lines)

        except Exception as e:
            self.log_cb('error', str(e), os.path.basename(self.path), line, 0)


    def run(self):
        lastS = 0
        speed = 0
        first = True
        x, y, z = 0, 0, 0

        with gzip.open('positions.gz', 'wb') as f1:
            with gzip.open('speeds.gz', 'wb') as f2:
                for move in self._run():
                    x = move.get('x', x)
                    y = move.get('y', y)
                    z = move.get('z', z)
                    rapid = move.get('rapid', False)
                    speed = move.get('s', speed)
                    s = struct.pack('<f', math.nan if rapid else speed)

                    if not first and s != lastS:
                        f1.write(p)
                        f2.write(s)

                    lastS = s
                    first = False
                    p = struct.pack('<fff', x, y, z)

                    f1.write(p)
                    f2.write(s)

        with open('meta.json', 'w') as f:
            meta = dict(
                time     = self.time,
                lines    = self.lines,
                maxSpeed = self.maxSpeed,
                bounds   = self.get_bounds(),
                messages = self.messages)

            json.dump(meta, f)


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
