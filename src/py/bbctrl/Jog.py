import inevent
from inevent.Constants import *


# Listen for input events
class Jog(inevent.JogHandler):
    def __init__(self, ctrl):
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

        self.processor = inevent.InEvent(ctrl.ioloop, self,
                                         types = "js kbd".split())


    def processed_events(self):
        if self.v != self.lastV:
            self.lastV = self.v

            v = ["{:6.5f}".format(x) for x in self.v]
            cmd = '$jog ' + ' '.join(v) + '\n'
            input_queue.put(cmd)


    def changed(self):
        if self.speed == 1: scale = 1.0 / 128.0
        if self.speed == 2: scale = 1.0 / 32.0
        if self.speed == 3: scale = 1.0 / 4.0
        if self.speed == 4: scale = 1.0

        self.v = [x * scale for x in self.axes]
