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

import json
import re
import logging
from collections import deque
import camotics.gplan as gplan # pylint: disable=no-name-in-module,import-error
import bbctrl.Cmd as Cmd

log = logging.getLogger('Planner')

reLogLine = re.compile(
    r'^(?P<level>[A-Z])[0-9 ]:((?P<where>[^:]+:\d+:\d+):)?(?P<msg>.*)$')


class Planner():
    def __init__(self, ctrl):
        self.ctrl = ctrl
        self.lastID = -1
        self.mode = 'idle'
        self.setq = deque()

        ctrl.state.add_listener(self.update)

        self.reset()


    def is_running(self): return self.planner.is_running()


    def get_config(self):
        state = self.ctrl.state

        # Axis mapping for enabled motors
        axis2motor = {}
        for i in range(4):
            if state.get('%dpm' % i, False):
                axis = 'xyzabc'[int(state.get('%dan' % i))]
                axis2motor[axis] = i

        def get_vector(name, scale = 1):
            v = {}
            for axis in 'xyzabc':
                if axis in axis2motor:
                    motor = axis2motor[axis]
                    value = state.get(str(motor) + name, None)
                    if value is not None:
                        v[axis] = value * scale
            return v

        # Starting position
        start = {}
        for axis in 'xyzabc':
            if not axis in axis2motor: continue
            value = state.get(axis + 'p', None)
            if value is not None: start[axis] = value

        config = {
            "start":     start,
            "max-vel":   get_vector('vm', 1000),
            "max-accel": get_vector('am', 1000000),
            "max-jerk":  get_vector('jm', 1000000),
            # TODO junction deviation & accel
            }

        log.info('Config:' + json.dumps(config))

        return config


    def update(self, update):
        if 'id' in update:
            id = update['id']
            self.planner.set_active(id)

            # Syncronize planner variables with execution id
            self.release_set_cmds(id)

        # Automatically unpause on seek hold
        if self.ctrl.state.get('xx', '') == 'HOLDING' and \
                self.ctrl.state.get('pr', '') == 'Switch found' and \
                self.planner.is_synchronizing():
            self.ctrl.avr.unpause()


    def release_set_cmds(self, id):
        self.lastID = id

        # Apply all set commands <= to ID and those that follow consecutively
        while len(self.setq) and self.setq[0][0] - 1 <= self.lastID:
            id, name, value = self.setq.popleft()
            self.ctrl.state.set(name, value)
            if id == self.lastID + 1: self.lastID = id


    def queue_set_cmd(self, id, name, value):
        log.info('Planner set(#%d, %s, %s)', id, name, value)
        self.setq.append((id, name, value))
        self.release_set_cmds(self.lastID)


    def restart(self):
        state = self.ctrl.state
        id = state.get('id')

        position = {}
        for axis in 'xyzabc':
            if state.has(axis + 'p'):
                position[axis] = state.get(axis + 'p')

        log.info('Planner restart: %d %s' % (id, json.dumps(position)))
        self.planner.restart(id, position)


    def get_var(self, name):
        value = 0
        if len(name) and name[0] == '_':
            value = self.ctrl.state.get(name[1:], 0)

        log.info('Get: %s=%s' % (name, value))
        return value


    def log(self, line):
        line = line.strip()
        m = reLogLine.match(line)
        if not m: return

        level = m.group('level')
        msg = m.group('msg')
        where = m.group('where')

        if where is not None: extra = dict(where = where)
        else: extra = None

        if   level == 'I': log.info    (msg, extra = extra)
        elif level == 'D': log.debug   (msg, extra = extra)
        elif level == 'W': log.warning (msg, extra = extra)
        elif level == 'E': log.error   (msg, extra = extra)
        elif level == 'C': log.critical(msg, extra = extra)
        else: log.error('Could not parse planner log line: ' + line)


    def mdi(self, cmd):
        if self.mode == 'gcode':
            raise Exception('Cannot issue MDI command while GCode running')

        log.info('MDI:' + cmd)
        self.planner.load_string(cmd, self.get_config())
        self.mode = 'mdi'


    def load(self, path):
        if self.mode != 'idle':
            raise Exception('Busy, cannot start new GCode program')

        log.info('GCode:' + path)
        self.planner.load('upload' + path, self.get_config())


    def reset(self):
        self.planner = gplan.Planner()
        self.planner.set_resolver(self.get_var)
        self.planner.set_logger(self.log, 1, 'LinePlanner:3')
        self.setq.clear()


    def _encode(self, block):
        type = block['type']

        if type == 'line':
            return Cmd.line(block['target'], block['exit-vel'],
                            block['max-accel'], block['max-jerk'],
                            block['times'])

        if type == 'set':
            name, value = block['name'], block['value']

            if name == 'line': self.queue_set_cmd(block['id'], name, value)
            if name == 'tool': return Cmd.tool(value)
            if name == 'speed': return Cmd.speed(value)
            if name[0:1] == '_' and name[1:2] in 'xyzabc' and \
                    name[2:] == '_home':
                return Cmd.set_position(name[1], value)

            if len(name) and name[0] == '_':
                self.queue_set_cmd(block['id'], name[1:], value)

            return

        if type == 'output':
            return Cmd.output(block['port'], int(float(block['value'])))

        if type == 'dwell': return Cmd.dwell(block['seconds'])
        if type == 'pause': return Cmd.pause(block['optional'])
        if type == 'seek':
            return Cmd.seek(block['switch'], block['active'], block['error'])

        raise Exception('Unknown planner type "%s"' % type)


    def encode(self, block):
        cmd = self._encode(block)
        if cmd is not None: return Cmd.set('id', block['id']) + '\n' + cmd


    def has_move(self): return self.planner.has_more()


    def next(self):
        try:
            while self.planner.has_more():
                cmd = self.planner.next()
                cmd = self.encode(cmd)
                if cmd is not None: return cmd

            if not self.is_running(): self.mode = 'idle'

        except Exception as e:
            self.reset()
            log.exception(e)
