################################################################################
#                                                                              #
#                 This file is part of the Buildbotics firmware.               #
#                                                                              #
#        Copyright (c) 2015 - 2020, Buildbotics LLC, All rights reserved.      #
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

import logging

from inevent.Constants import *


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
    def __init__(self, log = None):
        self.reset()

        self.log = log if log else logging.getLogger('inevent')


    def changed(self):
        self.log.info(axes_to_string(self.axes) + ' x {:d}'.format(self.speed))


    def up(self): self.log.debug('up')
    def down(self): self.log.debug('down')
    def left(self): self.log.debug('left')
    def right(self): self.log.debug('right')


    def reset(self):
        self.axes = [0.0, 0.0, 0.0, 0.0]
        self.speed = 3
        self.vertical_lock = 0
        self.horizontal_lock = 0


    def clear(self):
        self.reset()
        self.changed()


    def get_config(self, name): raise Exception('No configs')


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

        self.log.debug(event_to_string(event, state))

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
