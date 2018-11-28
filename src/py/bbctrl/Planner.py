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
import time
from collections import deque
import camotics.gplan as gplan # pylint: disable=no-name-in-module,import-error
import bbctrl.Cmd as Cmd
from bbctrl.CommandQueue import CommandQueue

log = logging.getLogger('Planner')

reLogLine = re.compile(
    r'^(?P<level>[A-Z])[0-9 ]:'
    r'((?P<file>[^:]+):)?'
    r'((?P<line>\d+):)?'
    r'((?P<column>\d+):)?'
    r'(?P<msg>.*)$')


class Planner():
    def __init__(self, ctrl):
        self.ctrl = ctrl
        self.cmdq = CommandQueue()

        ctrl.state.add_listener(self._update)

        self.reset(False)
        self._report_time()


    def is_busy(self): return self.is_running() or self.cmdq.is_active()
    def is_running(self): return self.planner.is_running()


    def get_config(self, mdi, with_limits, with_position = True):
        state = self.ctrl.state
        config = self.ctrl.config

        cfg = {
            'default-units': state.get('units', 'METRIC'),
            'max-vel':   state.get_axis_vector('vm', 1000),
            'max-accel': state.get_axis_vector('am', 1000000),
            'max-jerk':  state.get_axis_vector('jm', 1000000),
            'rapid-auto-off': config.get('rapid-auto-off'),
            # TODO junction deviation & accel
            }

        if with_position: cfg['position'] = state.get_position()

        if with_limits:
            minLimit = state.get_soft_limit_vector('tn', -math.inf)
            maxLimit = state.get_soft_limit_vector('tm', math.inf)

            # If max <= min then no limit
            for axis in 'xyzabc':
                if maxLimit[axis] <= minLimit[axis]:
                    minLimit[axis], maxLimit[axis] = -math.inf, math.inf

            cfg['min-soft-limit'] = minLimit
            cfg['max-soft-limit'] = maxLimit

        if not mdi:
            program_start = config.get('program-start')
            if program_start: cfg['program-start'] = program_start

        overrides = {}

        tool_change = config.get('tool-change')
        if tool_change: overrides['M6'] = tool_change

        program_end = config.get('program-end')
        if program_end: overrides['M2'] = program_end

        if overrides: cfg['overrides'] = overrides

        log.info('Config:' + json.dumps(cfg))

        return cfg


    def _update(self, update):
        if 'id' in update:
            id = update['id']
            self.planner.set_active(id) # Release planner commands
            self.cmdq.release(id)       # Synchronize planner variables


    def _get_var_cb(self, name, units):
        value = 0

        if len(name) and name[0] == '_':
            value = self.ctrl.state.get(name[1:], 0)
            if units == 'IMPERIAL': value /= 25.4 # Assume metric

        log.info('Get: %s=%s (units=%s)' % (name, value, units))

        return value


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
        if where: extra = dict(where = where)
        else: extra = None

        if   level == 'I': log.info    (msg, extra = extra)
        elif level == 'D': log.debug   (msg, extra = extra)
        elif level == 'W': log.warning (msg, extra = extra)
        elif level == 'E': log.error   (msg, extra = extra)
        elif level == 'C': log.critical(msg, extra = extra)
        else: log.error('Could not parse planner log line: ' + line)


    def _enqueue_set_cmd(self, id, name, value):
        log.info('set(#%d, %s, %s)', id, name, value)
        self.cmdq.enqueue(id, self.ctrl.state.set, name, value)


    def _report_time(self):
        state = self.ctrl.state.get('xx', '')

        if state in ['STOPPING', 'RUNNING'] and self.move_start:
            delta = time.time() - self.move_start
            if self.move_time < delta: delta = self.move_time
            plan_time = self.current_plan_time + delta

            self.ctrl.state.set('plan_time', round(plan_time))

        elif state != 'HOLDING': self.ctrl.state.set('plan_time', 0)

        self.ctrl.ioloop.call_later(1, self._report_time)


    def _plan_time_restart(self):
        self.plan_time = self.ctrl.state.get('plan_time', 0)


    def _update_time(self, plan_time, move_time):
        self.current_plan_time = plan_time
        self.move_time = move_time
        self.move_start = time.time()


    def _enqueue_line_time(self, block):
        if block.get('first', False) or block.get('seeking', False): return

        # Sum move times
        move_time = sum(block['times']) / 1000 # To seconds

        self.cmdq.enqueue(block['id'], self._update_time, self.plan_time,
                          move_time)

        self.plan_time += move_time


    def _enqueue_dwell_time(self, block):
        self.cmdq.enqueue(block['id'], self._update_time, self.plan_time,
                          block['seconds'])
        self.plan_time += block['seconds']


    def __encode(self, block):
        type, id = block['type'], block['id']

        if type != 'set': log.info('Cmd:' + json.dumps(block))

        if type == 'line':
            self._enqueue_line_time(block)
            return Cmd.line(block['target'], block['exit-vel'],
                            block['max-accel'], block['max-jerk'],
                            block['times'], block.get('speeds', []))

        if type == 'set':
            name, value = block['name'], block['value']

            if name == 'message':
                msg = dict(message = value)
                self.cmdq.enqueue(id, self.ctrl.msgs.broadcast, msg)

            if name in ['line', 'tool']: self._enqueue_set_cmd(id, name, value)
            if name == 'speed': return Cmd.speed(value)

            if len(name) and name[0] == '_':
                # Don't queue axis positions, can be triggered by new position
                if len(name) != 2 or name[1] not in 'xyzabc':
                    self._enqueue_set_cmd(id, name[1:], value)

            if name == '_feed': # Must come after _enqueue_set_cmd() above
                return Cmd.set_sync('if', 1 / value if value else 0)

            if name[0:1] == '_' and name[1:2] in 'xyzabc':
                if name[2:] == '_home': return Cmd.set_axis(name[1], value)

                if name[2:] == '_homed':
                    motor = self.ctrl.state.find_motor(name[1])
                    if motor is not None:
                        return Cmd.set_sync('%dh' % motor, value)

            return

        if type == 'input':
            # TODO handle timeout
            self.planner.synchronize(0) # TODO Fix this
            return Cmd.input(block['port'], block['mode'], block['timeout'])

        if type == 'output':
            return Cmd.output(block['port'], int(float(block['value'])))

        if type == 'dwell':
            self._enqueue_dwell_time(block)
            return Cmd.dwell(block['seconds'])

        if type == 'pause': return Cmd.pause(block['pause-type'])
        if type == 'seek':
            return Cmd.seek(block['switch'], block['active'], block['error'])
        if type == 'end': return '' # Sends id

        raise Exception('Unknown planner command "%s"' % type)


    def _encode(self, block):
        # Handle id wrapping by restricting ids to 16-bit
        # See CommandQueue id_less()
        block['id'] %= (1 << 16) - 1

        cmd = self.__encode(block)

        if cmd is not None:
            self.cmdq.enqueue(block['id'], None)
            return Cmd.set_sync('id', block['id']) + '\n' + cmd


    def reset_times(self):
        self.move_start = 0
        self.move_time = 0
        self.plan_time = 0
        self.current_plan_time = 0


    def reset(self, stop = True):
        if stop: self.ctrl.mach.stop()
        self.planner = gplan.Planner()
        self.planner.set_resolver(self._get_var_cb)
        self.planner.set_logger(self._log_cb, 1, 'LinePlanner:3')
        self.cmdq.clear()
        self.reset_times()
        self.ctrl.state.reset()


    def mdi(self, cmd, with_limits = True):
        log.info('MDI:' + cmd)
        self.planner.load_string(cmd, self.get_config(True, with_limits))
        self.reset_times()


    def load(self, path):
        log.info('GCode:' + path)
        self.planner.load(path, self.get_config(False, True))
        self.reset_times()


    def stop(self):
        try:
            self.planner.stop()
            self.cmdq.clear()

        except Exception as e:
            log.exception(e)
            self.reset()


    def restart(self):
        try:
            id = self.ctrl.state.get('id')
            position = self.ctrl.state.get_position()

            log.info('Planner restart: %d %s' % (id, json.dumps(position)))

            self.cmdq.clear()
            self.cmdq.release(id)
            self._plan_time_restart()
            self.planner.restart(id, position)

        except Exception as e:
            log.exception(e)
            self.stop()


    def next(self):
        try:
            while self.planner.has_more():
                cmd = self.planner.next()
                cmd = self._encode(cmd)
                if cmd is not None: return cmd

        except Exception as e:
            log.exception(e)
            self.stop()
