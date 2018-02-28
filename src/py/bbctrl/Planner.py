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
import math
import re
import logging
from collections import deque
import camotics.gplan as gplan # pylint: disable=no-name-in-module,import-error
import bbctrl.Cmd as Cmd
from bbctrl.CommandQueue import CommandQueue

log = logging.getLogger('Planner')

reLogLine = re.compile(
    r'^(?P<level>[A-Z])[0-9 ]:((?P<where>[^:]+:\d+:\d+):)?(?P<msg>.*)$')


class Planner():
    def __init__(self, ctrl):
        self.ctrl = ctrl
        self.cmdq = CommandQueue()

        ctrl.state.add_listener(self._update)

        self.reset()


    def is_busy(self): return self.is_running() or self.cmdq.is_active()
    def is_running(self): return self.planner.is_running()
    def is_synchronizing(self): return self.planner.is_synchronizing()


    def set_position(self, position):
        self.planner.set_position(position)


    def update_position(self):
        position = {}

        for axis in 'xyzabc':
            if not self.ctrl.state.is_axis_enabled(axis): continue
            value = self.ctrl.state.get(axis + 'p', None)
            if value is not None: position[axis] = value

        self.set_position(position)


    def _get_config_vector(self, name, scale):
        state = self.ctrl.state
        v = {}

        for axis in 'xyzabc':
            motor = state.find_motor(axis)

            if motor is not None and state.motor_enabled(motor):
                value = state.get(str(motor) + name, None)
                if value is not None: v[axis] = value * scale

        return v


    def _get_soft_limit(self, var, default):
        limit = self._get_config_vector(var, 1)

        for axis in 'xyzabc':
            if not axis in limit or not self.ctrl.state.is_axis_homed(axis):
                limit[axis] = default

        return limit


    def _get_config(self, mdi, with_limits):
        config = {
            'max-vel':   self._get_config_vector('vm', 1000),
            'max-accel': self._get_config_vector('am', 1000000),
            'max-jerk':  self._get_config_vector('jm', 1000000),
            # TODO junction deviation & accel
            }

        if with_limits:
            config['min-soft-limit'] = self._get_soft_limit('tn', -math.inf)
            config['max-soft-limit'] = self._get_soft_limit('tm', math.inf)

        if not mdi:
            program_start = self.ctrl.config.get('program-start')
            if program_start: config['program-start'] = program_start


        overrides = {}

        tool_change = self.ctrl.config.get('tool-change')
        if tool_change: overrides['M6'] = tool_change

        program_end = self.ctrl.config.get('program-end')
        if program_end: overrides['M2'] = program_end

        if overrides: config['overrides'] = overrides

        log.info('Config:' + json.dumps(config))

        return config


    def _update(self, update):
        if 'id' in update:
            id = update['id']
            self.planner.set_active(id)

            # Synchronize planner variables with execution id
            self.cmdq.release(id)


    def _get_var_cb(self, name):
        value = 0
        if len(name) and name[0] == '_':
            value = self.ctrl.state.get(name[1:], 0)

        log.info('Get: %s=%s' % (name, value))
        return value


    def _log_cb(self, line):
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



    def _enqueue_set_cmd(self, id, name, value):
        log.info('set(#%d, %s, %s)', id, name, value)
        self.cmdq.enqueue(id, True, self.ctrl.state.set, name, value)


    def __encode(self, block):
        log.info('Cmd:' + json.dumps(block))

        type, id = block['type'], block['id']

        if type == 'line':
            return Cmd.line(block['target'], block['exit-vel'],
                            block['max-accel'], block['max-jerk'],
                            block['times'])

        if type == 'set':
            name, value = block['name'], block['value']

            if name == 'message':
                self.cmdq.enqueue(
                    id, True, self.ctrl.msgs.broadcast, {'message': value})

            if name in ['line', 'tool']:
                self._enqueue_set_cmd(id, name, value)

            if name == 'speed': return Cmd.speed(value)

            if len(name) and name[0] == '_':
                self._enqueue_set_cmd(id, name[1:], value)

            if name[0:1] == '_' and name[1:2] in 'xyzabc':
                if name[2:] == '_home': return Cmd.set_axis(name[1], value)

                if name[2:] == '_homed':
                    motor = self.ctrl.state.find_motor(name[1])
                    if motor is not None: return Cmd.set('%dh' % motor, value)

            return

        if type == 'input':
            # TODO handle timeout
            self.planner.synchronize(0) # TODO Fix this
            return Cmd.input(block['port'], block['mode'], block['timeout'])

        if type == 'output':
            return Cmd.output(block['port'], int(float(block['value'])))

        if type == 'dwell': return Cmd.dwell(block['seconds'])
        if type == 'pause': return Cmd.pause(block['pause-type'])
        if type == 'seek':
            return Cmd.seek(block['switch'], block['active'], block['error'])

        raise Exception('Unknown planner command "%s"' % type)


    def _encode(self, block):
        cmd = self.__encode(block)

        if cmd is not None:
            self.cmdq.enqueue(block['id'], False, None)
            return Cmd.set('id', block['id']) + '\n' + cmd


    def reset(self):
        self.planner = gplan.Planner()
        self.planner.set_resolver(self._get_var_cb)
        self.planner.set_logger(self._log_cb, 1, 'LinePlanner:3')
        self.cmdq.clear()


    def mdi(self, cmd, with_limits = True):
        log.info('MDI:' + cmd)
        self.planner.load_string(cmd, self._get_config(True, with_limits))


    def load(self, path):
        log.info('GCode:' + path)
        self.planner.load(path, self._get_config(False, True))


    def stop(self):
        try:
            self.planner.stop()
            self.cmdq.clear()

        except Exception as e:
            log.exception(e)
            self.reset()


    def restart(self):
        try:
            state = self.ctrl.state
            id = state.get('id')

            position = {}
            for axis in 'xyzabc':
                if state.has(axis + 'p'):
                    position[axis] = state.get(axis + 'p')

            log.info('Planner restart: %d %s' % (id, json.dumps(position)))
            self.planner.restart(id, position)
            if self.planner.is_synchronizing(): self.planner.synchronize(1)

        except Exception as e:
            log.exception(e)
            self.reset()


    def has_move(self): return self.planner.has_more()


    def next(self):
        try:
            while self.planner.has_more():
                cmd = self.planner.next()
                cmd = self._encode(cmd)
                if cmd is not None: return cmd

        except Exception as e:
            log.exception(e)
            self.reset()
