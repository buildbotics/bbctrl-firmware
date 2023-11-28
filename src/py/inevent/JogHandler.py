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

import logging

from inevent.Constants import *


class JogHandler:
    def __init__(self, log = None):
        self.reset()
        self.log = log if log else logging.getLogger('inevent')


    def changed(self): pass
    def up(self):      pass
    def down(self):    pass
    def left(self):    pass
    def right(self):   pass


    def reset(self):
        self.axes = [0.0, 0.0, 0.0, 0.0]
        self.speed = 3
        self.vertical_lock = 0
        self.horizontal_lock = 0


    def clear(self):
        self.reset()
        self.changed()


    def get_config(self, type, event): raise Exception('Not implemented')
    def match_code(self, type, event): raise Exception('Not implemented')
    def has_code(self, type, event):   raise Exception('Not implemented')


    def __call__(self, event):
        if event.type not in [EV_ABS, EV_REL, EV_KEY]: return

        changed = False
        old_axes = list(self.axes)
        deadband = self.get_config('deadband', event)

        # Process event
        if event.type == EV_ABS and self.has_code('axes', event):
            axis = self.match_code('axes', event)

            self.axes[axis] = event.stream.abs[event.code]
            self.axes[axis] *= self.get_config('dir', event)[axis]

            v = abs(self.axes[axis])
            if v < deadband: self.axes[axis] = 0
            else:
                negative = self.axes[axis] < 0
                self.axes[axis] = (v - deadband) / (1 - deadband)
                if negative: self.axes[axis] *= -1

            if self.horizontal_lock and axis not in [0, 3]:
                self.axes[axis] = 0

            if self.vertical_lock and axis not in [1, 2]:
                self.axes[axis] = 0

            if old_axes[axis] != self.axes[axis]: changed = True

        elif event.type == EV_ABS and self.has_code('arrows', event):
            axis = self.match_code('arrows', event)

            if event.value < 0:
                if axis == 1: self.up()
                else: self.left()

            elif 0 < event.value:
                if axis == 1: self.down()
                else: self.right()

        elif event.type == EV_KEY and self.has_code('speed', event):
            old_speed = self.speed
            self.speed = self.match_code('speed', event) + 1
            if self.speed != old_speed: changed = True

        elif event.type == EV_KEY and self.has_code('lock', event):
            index = self.match_code('lock', event)

            self.horizontal_lock, self.vertical_lock = False, False

            if event.value:
                if index == 0: self.horizontal_lock = True
                if index == 1: self.vertical_lock = True

        self.log.info(str(event))

        if changed: self.changed()
