import inevent
from inevent.Constants import *


# Listen for input events
class Jog(inevent.JogHandler):
    def __init__(self, ctrl):
        self.ctrl = ctrl

        config = {
            "deadband": 0.1,
            "axes": [ABS_X, ABS_Y, ABS_RZ, ABS_Z],
            "arrows": [ABS_HAT0X, ABS_HAT0Y],
            "speed": [0x120, 0x121, 0x122, 0x123],
            "activate": [0x124, 0x126, 0x125, 0x127],
            }

        super().__init__(config)

        self.v = [0.0] * 4
        self.lastV = self.v
        self.callback()

        self.processor = inevent.InEvent(ctrl.ioloop, self,
                                         types = "js kbd".split())


    def callback(self):
        if self.v != self.lastV:
            self.lastV = self.v
            try:
                self.ctrl.avr.jog(self.v)
            except: pass

        self.ctrl.ioloop.call_later(0.25, self.callback)


    def changed(self):
        if self.speed == 1: scale = 1.0 / 128.0
        if self.speed == 2: scale = 1.0 / 32.0
        if self.speed == 3: scale = 1.0 / 4.0
        if self.speed == 4: scale = 1.0

        self.v = [x * scale for x in self.axes]
        self.v[ABS_Y] = -self.v[ABS_Y]
        self.v[ABS_Z] = -self.v[ABS_Z]
