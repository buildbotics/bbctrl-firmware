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

import logging
import traceback
import copy
import uuid
import bbctrl


log = logging.getLogger('State')


class StateSnapshot:
    def __init__(self, state):
        self.vars = copy.deepcopy(state.vars)

        for name in state.callbacks:
            if not name in self.vars:
                self.vars[name] = state.callbacks[name](name)

        self.motors = {}
        for axis in 'xyzabc':
            self.motors[axis] = state.find_motor(axis)


    def json(self): return dict(vars = self.vars, motors = self.motors)


    def resolve(self, name):
        # Resolve axis prefixes to motor numbers
        if 2 < len(name) and name[1] == '_' and name[0] in 'xyzabc':
            motor = self.motors[name[0]]
            if motor is not None: return str(motor) + name[2:]

        return name


    def get(self, name, default = None):
        name = self.resolve(name)

        if name in self.vars: return self.vars[name]
        if default is None: log.warning('State variable "%s" not found' % name)
        return default



class State(object):
    def __init__(self, ctrl):
        self.ctrl = ctrl
        self.callbacks = {}
        self.changes = {}
        self.listeners = []
        self.timeout = None
        self.machine_var_set = set()

        # Defaults
        self.vars = {
            'line': -1,
            'tool': 0,
            'feed': 0,
            'speed': 0,
        }

        # Add computed variable callbacks for each motor.
        #
        # NOTE, variable callbacks must return metric values only because
        # the planner will scale returned values when in imperial mode.
        for i in range(4):
            self.set_callback(str(i) + 'home_position',
                              lambda name, i = i: self.motor_home_position(i))
            self.set_callback(str(i) + 'home_travel',
                              lambda name, i = i: self.motor_home_travel(i))
            self.set_callback(str(i) + 'latch_backoff',
                              lambda name, i = i: self.motor_latch_backoff(i))
            self.set_callback(str(i) + 'zero_backoff',
                              lambda name, i = i: self.motor_zero_backoff(i))
            self.set_callback(str(i) + 'search_velocity',
                              lambda name, i = i: self.motor_search_velocity(i))
            self.set_callback(str(i) + 'latch_velocity',
                              lambda name, i = i: self.motor_latch_velocity(i))

        self.set_callback('metric', lambda name: 1 if self.is_metric() else 0)
        self.set_callback('imperial', lambda name: 0 if self.is_metric() else 1)

        self.set('sid', str(uuid.uuid4()))

        self.reset()


    def is_metric(self): return self.ctrl.config.get('units') == 'METRIC'


    def reset(self):
        # Unhome all motors
        for i in range(4): self.set('%dhomed' % i, False)

        # Zero offsets and positions
        for axis in 'xyzabc':
            self.set(axis + 'p', 0)
            self.set('offset_' + axis, 0)


    def _notify(self):
        if not self.changes: return

        for listener in self.listeners:
            try:
                listener(self.changes)

            except Exception as e:
                log.warning('Updating state listener: %s',
                            traceback.format_exc())

        self.changes = {}
        self.timeout = None


    def resolve(self, name):
        # Resolve axis prefixes to motor numbers
        if 2 < len(name) and name[1] == '_' and name[0] in 'xyzabc':
            motor = self.find_motor(name[0])
            if motor is not None: return str(motor) + name[2:]

        return name


    def has(self, name): return self.resolve(name) in self.vars
    def set_callback(self, name, cb): self.callbacks[self.resolve(name)] = cb


    def set(self, name, value):
        name = self.resolve(name)

        if not name in self.vars or self.vars[name] != value:
            self.vars[name] = value
            self.changes[name] = value

            # Trigger listener notify
            if self.timeout is None:
                self.timeout = self.ctrl.ioloop.call_later(0.25, self._notify)


    def update(self, update):
        for name, value in update.items():
            self.set(name, value)


    def get(self, name, default = None):
        name = self.resolve(name)

        if name in self.vars: return self.vars[name]
        if name in self.callbacks: return self.callbacks[name](name)
        if default is None: log.warning('State variable "%s" not found' % name)
        return default


    def snapshot(self): return StateSnapshot(self)


    def config(self, code, value):
        # Set machine variables via mach, others directly
        if code in self.machine_var_set: self.ctrl.mach.set(code, value)
        else: self.set(code, value)


    def add_listener(self, listener):
        log.info(self.vars)
        self.listeners.append(listener)
        listener(self.vars)


    def remove_listener(self, listener): self.listeners.remove(listener)


    def set_machine_vars(self, vars):
        # Record all machine vars, indexed or otherwise
        self.machine_var_set = set()
        for code, spec in vars.items():
            if 'index' in spec:
                for index in spec['index']:
                    self.machine_var_set.add(index + code)
            else: self.machine_var_set.add(code)


    def find_motor(self, axis):
        for motor in range(6):
            if not ('%dan' % motor) in self.vars: continue
            motor_axis = 'xyzabc'[self.vars['%dan' % motor]]
            if motor_axis == axis.lower() and self.vars.get('%dpm' % motor, 0):
                return motor


    def is_axis_homed(self, axis): return self.get('%s_homed' % axis, False)


    def is_axis_enabled(self, axis):
        motor = self.find_motor(axis)
        return motor is not None and self.motor_enabled(motor)


    def get_enabled_axes(self):
        axes = []

        for axis in 'xyzabc':
            if self.is_axis_enabled(axis):
                axes.append(axis)

        return axes


    def is_motor_faulted(self, motor):
        return self.get('%ddf' % motor, 0) & 0x1f


    def is_axis_faulted(self, axis):
        motor = self.find_motor(axis)
        return motor is not None and self.is_motor_faulted(motor)


    def axis_homing_mode(self, axis):
        motor = self.find_motor(axis)
        if motor is None: return 'disabled'
        return self.motor_homing_mode(motor)


    def axis_home_fail_reason(self, axis):
        motor = self.find_motor(axis)
        if motor is None: return 'Not mapped to motor'
        if not self.motor_enabled(motor): return 'Motor disabled'

        mode = self.motor_homing_mode(motor)

        if mode == 'manual': return 'Configured for manual homing'

        if mode == 'switch-min' and not int(self.get(axis + '_ls')):
            return 'Configured for min switch but switch is disabled'

        if mode == 'switch-max' and not int(self.get(axis + '_xs')):
            return 'Configured for max switch but switch is disabled'

        softMin = int(self.get(axis + '_tn'))
        softMax = int(self.get(axis + '_tm'))
        if softMax <= softMin + 1:
            return 'max-soft-limit must be at least 1mm greater ' \
                'than min-soft-limit'


    def motor_enabled(self, motor):
        return bool(int(self.vars.get('%dpm' % motor, 0)))


    def motor_homing_mode(self, motor):
        mode = str(self.vars.get('%dho' % motor, 0))
        if mode == '0': return 'manual'
        if mode == '1': return 'switch-min'
        if mode == '2': return 'switch-max'
        raise Exception('Unrecognized homing mode "%s"' % mode)


    def motor_home_direction(self, motor):
        mode = self.motor_homing_mode(motor)
        if mode == 'switch-min': return -1
        if mode == 'switch-max': return 1
        return 0 # Disabled


    def motor_home_position(self, motor):
        mode = self.motor_homing_mode(motor)
        # Return soft limit positions
        if mode == 'switch-min': return self.vars['%dtn' % motor]
        if mode == 'switch-max': return self.vars['%dtm' % motor]
        return 0 # Disabled


    def motor_home_travel(self, motor):
        tmin = self.get(str(motor) + 'tm')
        tmax = self.get(str(motor) + 'tn')
        hdir = self.motor_home_direction(motor)

        # (travel_max - travel_min) * 1.5 * home_dir
        return (tmin - tmax) * 1.5 * hdir


    def motor_latch_backoff(self, motor):
        lb = self.get(str(motor) + 'lb')
        hdir = self.motor_home_direction(motor)
        return -(lb * hdir) # -latch_backoff * home_dir


    def motor_zero_backoff(self, motor):
        zb = self.get(str(motor) + 'zb')
        hdir = self.motor_home_direction(motor)
        return -(zb * hdir) # -zero_backoff * home_dir


    def motor_search_velocity(self, motor):
        return 1000 * self.get(str(motor) + 'sv')


    def motor_latch_velocity(self, motor):
        return 1000 * self.get(str(motor) + 'lv')
