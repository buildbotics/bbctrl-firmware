import logging

import bbctrl


log = logging.getLogger('Ctrl')


class Ctrl(object):
    def __init__(self, args, ioloop):
        self.args = args
        self.ioloop = ioloop

        self.config = bbctrl.Config(self)
        self.web = bbctrl.Web(self)
        self.avr = bbctrl.AVR(self)
        self.jog = bbctrl.Jog(self)
        self.lcd = bbctrl.LCD(self)

        self.avr.connect()
