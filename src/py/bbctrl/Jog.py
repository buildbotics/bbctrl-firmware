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

import inevent
from inevent.Constants import *
import copy

__all__ = ['Jog']


config = {
    'deadband': 0.15,
    'dir':    [1, -1, -1, 1],
    'arrows': [[ABS_HAT0X], [ABS_HAT0Y]],
    'speed':  [[0x133, 0x120], [0x130, 0x121], [0x131, 0x122], [0x134, 0x123]],
    'lock':   [[0x136, 0x124], [0x137, 0x125]],
}

configA = copy.deepcopy(config)
configB = copy.deepcopy(config)
configA['axes'] = [[ABS_X], [ABS_Y], [ABS_RZ], [ABS_Z]]
configB['axes'] = [[ABS_X], [ABS_Y], [ABS_RY], [ABS_RX]]


# Listen for input events
class Jog(inevent.JogHandler):
    def __init__(self, ctrl):
        super().__init__(ctrl.log.get('JogHandler'))

        self.ctrl = ctrl
        self.log  = ctrl.log.get('Jog')

        self.v = [0.0] * 4
        self.lastV = self.v
        self.update()

        self.processor = inevent.InEvent(
            ctrl.ioloop, ctrl.udevev, self, types = ['js'],
            log = ctrl.log.get('InEvent'))


    def _get_config(self, event):
        # There are two types of controller.  One uses axes 0, 1, 2 & 5 and
        # the other uses 0, 1, 3 & 4 for X, Y, Z and A.  However, the second
        # type also uses axes 2 & 5 for the front analog trigger buttons.
        # We differentiate between the two by looking for an unused axis 3.
        if event.stream.absScale[3].max == 0: return configA
        return configB


    # From inevent.JogHandler
    def match_code(self, type, event):
        config = self._get_config(event)

        if not type in config: return None

        index = 0
        for codes in config[type]:
            if event.code in codes: return index
            index += 1


    def get_config(self, type, event): return self._get_config(event)[type]


    def has_code(self, type, event):
        return self.match_code(type, event) is not None


    def up(self):    self.ctrl.lcd.page_up()
    def down(self):  self.ctrl.lcd.page_down()
    def left(self):  self.ctrl.lcd.page_left()
    def right(self): self.ctrl.lcd.page_right()


    def update(self):
        if self.v != self.lastV:
            self.lastV = self.v
            try:
                axes = {}
                for i in range(len(self.v)): axes["xyzabc"[i]] = self.v[i]
                self.ctrl.mach.jog(axes)

            except Exception as e:
                self.log.warning('Jog: %s', e)

        self.ctrl.ioloop.call_later(0.25, self.update)


    def changed(self):
        scale = 1.0
        if self.speed == 1: scale = 1.0 / 128.0
        if self.speed == 2: scale = 1.0 / 32.0
        if self.speed == 3: scale = 1.0 / 4.0

        self.v = [x * scale for x in self.axes]
