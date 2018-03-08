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

from inevent.Constants import *


log = logging.getLogger('inevent')
log.setLevel(logging.INFO)


def axes_to_string(axes):
    s = ''
    for axis in axes:
        if s: s += ', '
        else: s = '('
        s += '{:6.3f}'.format(axis)
    return s + ')'


def event_to_string(event, state):
    s = '{} {}: '.format(event.get_source(), event.get_type_name())

    if event.type == EV_ABS:
        s += axes_to_string(state.get_joystick3d()) + ' ' + \
            axes_to_string(state.get_joystickR3d()) + ' ' + \
            '({:2.0f}, {:2.0f}) '.format(*state.get_hat())

    if event.type == EV_REL:
        s += '({:d}, {:d}) '.format(*state.get_mouse()) + \
            '({:d}, {:d})'.format(*state.get_wheel())

    if event.type == EV_KEY:
        state = 'pressed' if event.value else 'released'
        s += '0x{:x} {}'.format(event.code, state)

    return s


class JogHandler:
    def __init__(self, config):
        self.config = config
        self.reset()


    def changed(self):
        log.info(axes_to_string(self.axes) + ' x {:d}'.format(self.speed))


    def up(self): log.debug('up')
    def down(self): log.debug('down')
    def left(self): log.debug('left')
    def right(self): log.debug('right')


    def reset(self):
        self.axes = [0.0, 0.0, 0.0, 0.0]
        self.speed = 3
        self.vertical_lock = 0
        self.horizontal_lock = 0


    def clear(self):
        self.reset()
        self.changed()


    def get_config(self, name):
        if name in self.config: return self.config[name]
        return self.config['default']


    def event(self, event, state, dev_name):
        if event.type not in [EV_ABS, EV_REL, EV_KEY]: return

        config = self.get_config(dev_name)
        changed = False

        # Process event
        if event.type == EV_ABS and event.code in config['axes']:
            pass

        elif event.type == EV_ABS and event.code in config['arrows']:
            axis = config['arrows'].index(event.code)

            if event.value < 0:
                if axis == 1: self.up()
                else: self.left()

            elif 0 < event.value:
                if axis == 1: self.down()
                else: self.right()

        elif event.type == EV_KEY and event.code in config['speed']:
            old_speed = self.speed
            self.speed = config['speed'].index(event.code) + 1
            if self.speed != old_speed: changed = True

        elif event.type == EV_KEY and event.code in config['lock']:
            index = config['lock'].index(event.code)

            self.horizontal_lock, self.vertical_lock = False, False

            if event.value:
                if index == 0: self.horizontal_lock = True
                if index == 1: self.vertical_lock = True

        log.debug(event_to_string(event, state))

        # Update axes
        old_axes = list(self.axes)

        for axis in range(4):
            self.axes[axis] = event.stream.state.abs[config['axes'][axis]]
            self.axes[axis] *= config['dir'][axis]

            if abs(self.axes[axis]) < config['deadband']:
                self.axes[axis] = 0

            if self.horizontal_lock and axis not in [0, 3]:
                self.axes[axis] = 0

            if self.vertical_lock and axis not in [1, 2]:
                self.axes[axis] = 0

        if old_axes != self.axes: changed = True

        if changed: self.changed()
