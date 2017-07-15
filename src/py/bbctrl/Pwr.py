import logging

import bbctrl

log = logging.getLogger('PWR')


# Must match regs in pwr firmware
TEMP_REG        = 0
VIN_REG         = 1
VOUT_REG        = 2
MOTOR_REG       = 3
LOAD1_REG       = 4
LOAD2_REG       = 5


class Pwr():
    def __init__(self, ctrl):
        self.ctrl = ctrl

        self.i2c_addr = ctrl.args.pwr_addr
        self.regs = [-1] * 6
        self.lcd_page = ctrl.lcd.add_new_page()

        self._update()


    def get_reg(self, i): return self.regs[i]


    def _update(self):
        update = {}

        try:
            for i in range(len(self.regs)):
                value = self.ctrl.i2c.read_word(self.i2c_addr + i)

                if i == TEMP_REG: value -= 273
                else: value /= 100.0

                if self.regs[i] != value:
                    key = ['temp', 'vin', 'vout', 'motor', 'load1', 'load2'][i]
                    update[key] = value
                    self.regs[i] = value

        except Exception as e:
            log.warning('Pwr communication failed: %s' % e)
            self.ctrl.ioloop.call_later(1, self._update)
            return

        self.lcd_page.text('%3dC'       % self.regs[TEMP_REG], 0, 0)
        self.lcd_page.text('%5.1fV In'  % self.regs[VIN_REG],  0, 1)
        self.lcd_page.text('%5.1fV Out' % self.regs[VOUT_REG], 0, 2)

        self.lcd_page.text('%5.1fA Mot' % self.regs[MOTOR_REG], 10, 0)
        self.lcd_page.text('%5.1fA Ld1' % self.regs[LOAD1_REG], 10, 1)
        self.lcd_page.text('%5.1fA Ld2' % self.regs[LOAD2_REG], 10, 2)

        if len(update): self.ctrl.web.broadcast(update)

        self.ctrl.ioloop.call_later(0.25, self._update)
