################################################################################
#                                                                              #
#                 This file is part of the Buildbotics firmware.               #
#                                                                              #
#        Copyright (c) 2015 - 2026, Buildbotics LLC, All rights reserved.      #
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

import traceback
import copy
import uuid
import os
import time

from . import util

__all__ = ['State']


class State(object):
    def __init__(self, ctrl):
        self.ctrl = ctrl
        self.log = ctrl.log.get('State')

        self.callbacks = {}
        self.changes = {}
        self.listeners = []
        self.timeout = None
        self.machine_var_set = set()
        self.message_id = 0

        # Defaults
        self.vars = {
            'line':      -1,
            'messages':  [],
            'tool':      0,
            'feed':      0,
            'speed':     0,
            'sid':       str(uuid.uuid4()),
            'demo':      ctrl.args.demo,
            'rpi_model': util.get_model(),
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

        self.set_callback('timestamp', lambda name: time.time())

        self.reset()


    def init(self):
        # Initialize units
        metric = self.ctrl.config.get('units', 'METRIC').upper() == 'METRIC'
        if not 'metric' in self.vars: self.set('metric', metric)
        if not 'imperial' in self.vars: self.set('imperial', not metric)


    def reset(self):
        # Clear active program
        self.set('active_program', '')

        # Unhome all motors
        for i in range(4): self.set('%dhomed' % i, 0)

        # Zero offsets and positions
        for axis in 'xyzabc':
            self.set(axis + 'p', 0)
            self.set('offset_' + axis, 0)


    def ack_message(self, id):
        self.log.info('Message %d acknowledged' % id)
        msgs = self.vars['messages']
        msgs = list(filter(lambda m: id < m['id'], msgs))
        self.set('messages', msgs)


    def add_message(self, text):
        msg = dict(text = text, id = self.message_id)
        self.message_id += 1
        msgs = self.vars['messages']
        msgs = msgs + [msg] # It's important we make a new list here
        self.set('messages', msgs)


    def _notify(self):
        if not self.changes: return

        for listener in self.listeners:
            try:
                listener(self.changes)

            except Exception as e:
                self.log.warning('Updating state listener: %s',
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
        if default is None:
            self.log.warning('State variable "%s" not found' % name)

        return default


    def is_estopped(self): return self.get('xx', '') == 'ESTOPPED'


    def snapshot(self):
        vars = copy.deepcopy(self.vars)

        for name in self.callbacks:
            if not name in vars:
                vars[name] = self.callbacks[name](name)

        axis_motors = {axis: self.find_motor(axis) for axis in 'xyzabc'}
        axis_vars = {}

        for name, value in vars.items():
            if name[0] in '0123':
                motor = int(name[0])

                for axis in 'xyzabc':
                    if motor == axis_motors[axis]:
                        axis_vars[axis + '_' + name[1:]] = value

        vars.update(axis_vars)

        return vars


    def config(self, code, value):
        # Set machine variables via mach, others directly
        if code in self.machine_var_set: self.ctrl.mach.set(code, value)
        else: self.set(code, value)


    def add_listener(self, listener):
        self.listeners.append(listener)
        listener(self.vars)


    def remove_listener(self, listener): self.listeners.remove(listener)


    def set_machine_vars(self, vars):
        # Record all machine vars, indexed or otherwise
        self.machine_var_set = set()
        for code, spec in vars.items():
            if 'index' in spec:
                for index in spec['index']:
                    self.machine_var_set.add(str(index) + code)
            else: self.machine_var_set.add(code)


    def get_position(self):
        position = {}

        for axis in 'xyzabc':
            if self.is_axis_enabled(axis) and self.has(axis + 'p'):
                position[axis] = self.get(axis + 'p')

        return position


    def get_axis_vector(self, name, scale = 1):
        v = {}

        for axis in 'xyzabc':
            motor = self.find_motor(axis)

            if motor is not None and self.motor_enabled(motor):
                value = self.get(str(motor) + name, None)
                if value is not None: v[axis] = value * scale

        return v


    def get_soft_limit_vector(self, var, default):
        limit = self.get_axis_vector(var, 1)

        for axis in 'xyzabc':
            if not axis in limit or not self.is_axis_homed(axis):
                limit[axis] = default

        return limit


    def find_motor(self, axis):
        for motor in range(4):
            if not ('%dan' % motor) in self.vars: continue
            motor_axis = 'xyzabc'[self.vars['%dan' % motor]]
            if motor_axis == axis.lower() and self.vars.get('%dme' % motor, 0):
                return motor


    def is_axis_homed(self, axis): return self.get('%s_homed' % axis, 0)


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

        # TODO check for pin configured for required homing switch function

        softMin = int(self.get(axis + '_tn', 0))
        softMax = int(self.get(axis + '_tm', 0))
        if softMax < softMin + 1:
            return 'max-soft-limit must be at least 1mm greater ' \
                'than min-soft-limit'


    def motor_enabled(self, motor):
        return bool(int(self.vars.get('%dme' % motor, 0)))


    def motor_homing_mode(self, motor):
        mode = str(self.vars.get('%dho' % motor, 0))
        if mode == '0': return 'manual'
        if mode == '1': return 'switch-min'
        if mode == '2': return 'switch-max'
        if mode == '3': return 'stall-min'
        if mode == '4': return 'stall-max'
        raise Exception('Unrecognized homing mode "%s"' % mode)


    def motor_home_direction(self, motor):
        mode = self.motor_homing_mode(motor)
        if mode.endswith('-min'): return -1
        if mode.endswith('-max'): return 1
        return 0 # Disabled


    def motor_home_position(self, motor):
        mode = self.motor_homing_mode(motor)
        # Return soft limit positions
        if mode.endswith('-min'): return self.vars['%dtn' % motor]
        if mode.endswith('-max'): return self.vars['%dtm' % motor]
        return 0 # Disabled


    def motor_home_travel(self, motor):
        tmin = self.get(str(motor) + 'tm', 0)
        tmax = self.get(str(motor) + 'tn', 0)
        hdir = self.motor_home_direction(motor)

        # (travel_max - travel_min) * 1.5 * home_dir
        return (tmin - tmax) * 1.5 * hdir


    def motor_latch_backoff(self, motor):
        lb = self.get(str(motor) + 'lb', 0)
        hdir = self.motor_home_direction(motor)
        return -(lb * hdir) # -latch_backoff * home_dir


    def motor_zero_backoff(self, motor):
        zb = self.get(str(motor) + 'zb', 0)
        hdir = self.motor_home_direction(motor)
        return -(zb * hdir) # -zero_backoff * home_dir


    def motor_search_velocity(self, motor):
        return 1000 * self.get(str(motor) + 'sv', 0)


    def motor_latch_velocity(self, motor):
        return 1000 * self.get(str(motor) + 'lv', 0)


    def get_axis_switch(self, axis, side):
        axis = axis.lower()

        if not axis in 'xyzabc':
            raise Exception('Unsupported switch "%s-%s"' % (axis, side))

        if not self.is_axis_enabled(axis):
            raise Exception('Switch "%s-%s" axis not enabled' % (axis, side))

        motor = self.find_motor(axis)

        # The following must match the switch ID enum in avr/src/switch.h
        hmode = self.motor_homing_mode(motor)
        if hmode.startswith('stall-'): return motor + 10
        return 2 * motor + 2 + (0 if side.lower() == 'min' else 1)


    def get_switch_id(self, switch):
        # TODO Support other input switches in CAMotics gcode/machine/PortType.h
        switch = switch.lower()
        if switch == 'probe': return 1
        if switch[1:] == '-min': return self.get_axis_switch(switch[0], 'min')
        if switch[1:] == '-max': return self.get_axis_switch(switch[0], 'max')
        raise Exception('Unsupported switch "%s"' % switch)
