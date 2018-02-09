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
VDD_REG         = 6
FLAGS_REG       = 7

UNDER_VOLTAGE_FLAG     = 1 << 0
OVER_VOLTAGE_FLAG      = 1 << 1
OVER_CURRENT_FLAG      = 1 << 2
MEASUREMENT_ERROR_FLAG = 1 << 3
SHUNT_OVERLOAD_FLAG    = 1 << 4

reg_names = 'temp vin vout motor load1 load2 vdd pwr_flags'.split()


class Pwr():
    def __init__(self, ctrl):
        self.ctrl = ctrl

        self.i2c_addr = ctrl.args.pwr_addr
        self.regs = [-1] * 8
        self.lcd_page = ctrl.lcd.add_new_page()

        self._update()


    def get_reg(self, i): return self.regs[i]


    def error(self):
        flags = self.regs[FLAGS_REG]
        errors = []

        # Decode error flags
        if flags & UNDER_VOLTAGE_FLAG:     errors.append('under voltage')
        if flags & OVER_VOLTAGE_FLAG:      errors.append('over voltage')
        if flags & OVER_CURRENT_FLAG:      errors.append('over current')
        if flags & MEASUREMENT_ERROR_FLAG: errors.append('measurement error')
        if flags & SHUNT_OVERLOAD_FLAG:    errors.append('shunt overload')

        # Report errors
        if errors: log.error('Power fault: ' + ', '.join(errors))


    def _update(self):
        update = {}

        try:
            for i in range(len(self.regs)):
                value = self.ctrl.i2c.read_word(self.i2c_addr + i)

                if i == TEMP_REG: value -= 273
                else: value /= 100.0

                key = reg_names[i]
                self.ctrl.state.set(key, value)

                if self.regs[i] != value:
                    update[key] = value
                    self.regs[i] = value

                    if i == FLAGS_REG and value: self.error()

        except Exception as e:
            log.warning('Pwr communication failed: %s' % e)
            self.ctrl.ioloop.call_later(1, self._update)
            return

        self.lcd_page.text('%3dC   Tmp'   % self.regs[TEMP_REG], 0, 0)
        self.lcd_page.text('%5.1fV  In'   % self.regs[VIN_REG],  0, 1)
        self.lcd_page.text('%5.1fV Out'   % self.regs[VOUT_REG], 0, 2)
        self.lcd_page.text(' %02d    Flg' % self.regs[FLAGS_REG], 0, 3)

        self.lcd_page.text('%5.1fA Mot' % self.regs[MOTOR_REG], 10, 0)
        self.lcd_page.text('%5.1fA Ld1' % self.regs[LOAD1_REG], 10, 1)
        self.lcd_page.text('%5.1fA Ld2' % self.regs[LOAD2_REG], 10, 2)
        self.lcd_page.text('%5.1fA Vdd' % self.regs[VDD_REG],   10, 3)

        if len(update): self.ctrl.state.update(update)

        self.ctrl.ioloop.call_later(0.25, self._update)
