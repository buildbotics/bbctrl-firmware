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

import inevent
from inevent.Constants import *


# Listen for input events
class Jog(inevent.JogHandler):
    def __init__(self, ctrl):
        self.ctrl = ctrl
        self.log = ctrl.log.get('Jog')

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

        self.processor = inevent.InEvent(ctrl.ioloop, self, types = ['js'])


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
                self.log.warning('Jog: %s', e)

        self.ctrl.ioloop.call_later(0.25, self.callback)


    def changed(self):
        scale = 1.0
        if self.speed == 1: scale = 1.0 / 128.0
        if self.speed == 2: scale = 1.0 / 32.0
        if self.speed == 3: scale = 1.0 / 4.0

        self.v = [x * scale for x in self.axes]
