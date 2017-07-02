import logging

import bbctrl

log = logging.getLogger('PWR')


# Must match regs in pwr firmare
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
        self.regs = [0] * 6
        self.lcd_page = ctrl.lcd.add_new_page()

        self._update()


    def get_reg(self, i): return self.regs[i]


    def _update(self):
        try:
            for i in range(len(self.regs)):
                value = self.ctrl.i2c.read_word(self.i2c_addr + i)

                if i == TEMP_REG: self.regs[TEMP_REG] = value - 273
                else: self.regs[i] = value / 100.0

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

        self.ctrl.ioloop.call_later(0.25, self._update)
