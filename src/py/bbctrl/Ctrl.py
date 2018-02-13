import logging
import subprocess

import bbctrl


log = logging.getLogger('Ctrl')


class IPPage(bbctrl.LCDPage):
    def update(self):
        p = subprocess.Popen(['hostname', '-I'], stdout = subprocess.PIPE)
        ips = p.communicate()[0].decode('utf-8').split()

        p = subprocess.Popen(['hostname'], stdout = subprocess.PIPE)
        hostname = p.communicate()[0].decode('utf-8').strip()

        self.clear()

        self.text('Host: %s' % hostname[0:14], 0, 0)

        for i in range(min(3, len(ips))):
            self.text('IP: %s' % ips[i], 0, i + 1)


    def activate(self): self.update()


class Ctrl(object):
    def __init__(self, args, ioloop):
        self.args = args
        self.ioloop = ioloop

        self.msgs = bbctrl.Messages(self)
        self.state = bbctrl.State(self)
        self.config = bbctrl.Config(self)
        self.web = bbctrl.Web(self)

        try:
            self.planner = bbctrl.Planner(self)
            self.i2c = bbctrl.I2C(args.i2c_port)
            self.lcd = bbctrl.LCD(self)
            self.avr = bbctrl.AVR(self)
            self.jog = bbctrl.Jog(self)
            self.pwr = bbctrl.Pwr(self)

            self.avr.connect()

            self.lcd.add_new_page(IPPage(self.lcd))

        except Exception as e: log.exception(e)
