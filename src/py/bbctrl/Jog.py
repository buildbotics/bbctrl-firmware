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

import inevent
from inevent.Constants import *
import logging

log = logging.getLogger('Jog')


# Listen for input events
class Jog(inevent.JogHandler):
    def __init__(self, ctrl):
        self.ctrl = ctrl

        config = {
            "Logitech Logitech RumblePad 2 USB": {
                "deadband": 0.1,
                "axes":     [ABS_X, ABS_Y, ABS_RZ, ABS_Z],
                "dir":      [1, -1, -1, 1],
                "arrows":   [ABS_HAT0X, ABS_HAT0Y],
                "speed":    [0x120, 0x121, 0x122, 0x123],
                "lock":     [0x124, 0x125],
            },

            "default": {
                "deadband": 0.1,
                "axes":     [ABS_X, ABS_Y, ABS_RY, ABS_RX],
                "dir":      [1, -1, -1, 1],
                "arrows":   [ABS_HAT0X, ABS_HAT0Y],
                "speed":    [0x133, 0x130, 0x131, 0x134],
                "lock":     [0x136, 0x137],
            }
        }

        super().__init__(config)

        self.v = [0.0] * 4
        self.lastV = self.v
        self.callback()

        self.processor = inevent.InEvent(ctrl.ioloop, self,
                                         types = 'js kbd'.split())


    def up(self): self.ctrl.lcd.page_up()
    def down(self): self.ctrl.lcd.page_down()
    def left(self): self.ctrl.lcd.page_left()
    def right(self): self.ctrl.lcd.page_right()


    def callback(self):
        if self.v != self.lastV:
            self.lastV = self.v
            try:
                axes = {}
                for i in range(len(self.v)): axes["xyzabc"[i]] = self.v[i]
                self.ctrl.mach.jog(axes)

            except Exception as e:
                log.warning('Jog: %s', e)

        self.ctrl.ioloop.call_later(0.25, self.callback)


    def changed(self):
        scale = 1.0
        if self.speed == 1: scale = 1.0 / 128.0
        if self.speed == 2: scale = 1.0 / 32.0
        if self.speed == 3: scale = 1.0 / 4.0

        self.v = [x * scale for x in self.axes]
