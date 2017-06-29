import logging

import bbctrl


log = logging.getLogger('Ctrl')


class Ctrl(object):
    def __init__(self, args, ioloop):
        self.args = args
        self.ioloop = ioloop

        self.i2c = bbctrl.I2C(args.i2c_port)
        self.config = bbctrl.Config(self)
        self.lcd = bbctrl.LCD(self)
        self.web = bbctrl.Web(self)
        self.avr = bbctrl.AVR(self)
        self.jog = bbctrl.Jog(self)
        self.pwr = bbctrl.Pwr(self)

        self.avr.connect()
