################################################################################
#                                                                              #
#                 This file is part of the Buildbotics firmware.               #
#                                                                              #
#        Copyright (c) 2015 - 2021, Buildbotics LLC, All rights reserved.      #
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

import bbctrl
import bbctrl.Cmd as Cmd


# Must match regs in pwr firmware
TEMP_REG     = 0
VIN_REG      = 1
VOUT_REG     = 2
MOTOR_REG    = 3
LOAD1_REG    = 4
LOAD2_REG    = 5
VDD_REG      = 6
FLAGS_REG    = 7
VERSION_REG  = 8

reg_names = 'temp vin vout motor load1 load2 vdd pwr_flags pwr_version'.split()


def version_less(a, b):
    a, b = [[int(y) for y in x.split('.')] for x in (a, b)]
    return a < b


class FD(object):
    def __init__(self, bit, minVersion, maxVersion, name, description):
        self.mask = 1 << bit
        self.minVersion = minVersion
        self.maxVersion = maxVersion
        self.name = name
        self.description = description


    def valid_for_version(self, version):
        if version_less(version, self.minVersion): return False
        if not version_less(self.minVersion, self.maxVersion): return True
        return not version_less(self.maxVersion, version)


# Must be kept in sync with pwr firmware
flag_defs = [
    FD(0,  '0.0', '0.0', 'under_voltage', 'Device under voltage'),
    FD(1,  '0.0', '0.0', 'over_voltage', 'Device over voltage'),
    FD(2,  '0.0', '0.0', 'over_current', 'Device total current limit exceeded'),
    FD(3,  '0.0', '0.0', 'sense_error', 'Power sense error'),
    FD(4,  '0.0', '0.0', 'shunt_overload', 'Power shunt overload'),
    FD(5,  '0.0', '0.0', 'motor_overload', 'Motor power overload'),
    FD(6,  '0.0', '0.8', 'load1_shutdown', 'Load 1 over temperature shutdown'),
    FD(6,  '1.1', '0.0', 'gate_error', 'Motor power gate not working'),
    FD(7,  '0.0', '0.8', 'load2_shutdown', 'Load 2 over temperature shutdown'),
    FD(8,  '0.0', '0.0', 'motor_under_voltage', 'Motor under voltage'),
    FD(9,  '0.0', '0.0', 'motor_voltage_sense_error',
       'Motor voltage sense error'),
    FD(10, '0.0', '0.0', 'motor_current_sense_error',
       'Motor current sense error'),
    FD(11, '0.0', '0.8', 'load1_sense_error', 'Load1 sense error'),
    FD(12, '0.0', '0.8', 'load2_sense_error', 'Load2 sense error'),
    FD(13, '0.0', '0.8', 'vdd_current_sense_error', 'Vdd current sense error'),
    FD(14, '0.0', '0.0', 'shutdown', 'Power shutdown'),
    FD(15, '0.0', '0.0', 'shunt_error', 'Shunt error'),
]


class Pwr():
    def __init__(self, ctrl):
        self.ctrl = ctrl
        self.log = ctrl.log.get('Pwr')

        self.i2c_old = False
        self.i2c_confirmed_new = False

        self.regs = [-1] * 9
        self.lcd_page = ctrl.lcd.add_new_page()
        self.failures = 0

        self._update_cb(False)


    def check_fault(self, fd, status):
        if not fd.valid_for_version(self.regs[VERSION_REG]): return

        if (not self.ctrl.state.has(fd.name) or
            status != self.ctrl.state.get(fd.name)):
            self.ctrl.state.set(fd.name, status)

            if status:
                self.log.warning(fd.description)
                if fd.name == 'power_shutdown':
                    self.ctrl.mach.i2c_command(Cmd.SHUTDOWN)


    def check_faults(self):
        flags = self.regs[FLAGS_REG]

        for fd in flag_defs:
            self.check_fault(fd, bool(fd.mask & flags))


    def _update_cb(self, now = True):
        if now: self._update()
        self.ctrl.ioloop.call_later(1, self._update_cb)


    def _update(self):
        update = {}

        try:
            for i in reversed(range(len(self.regs))):
                if self.i2c_old: value = self.ctrl.i2c.read_word(0x60 + i, 0)
                else: value = self.ctrl.i2c.read_word(0x5f, i, pec = True)

                if value is None: return # Handle lack of i2c port

                if not self.i2c_old: self.i2c_confirmed_new = True

                if i == TEMP_REG: value -= 273
                elif i == VERSION_REG:
                    value = '%u.%u' % (value >> 8, value & 0xff)
                elif i == FLAGS_REG: pass
                else: value /= 100.0

                key = reg_names[i]
                self.ctrl.state.set(key, value)

                if self.regs[i] != value:
                    update[key] = value
                    self.regs[i] = value

                if i == FLAGS_REG: self.check_faults()

        except Exception as e:
            if not self.i2c_confirmed_new:
                self.i2c_old = not self.i2c_old

            # Older pwr firmware does not have regs > 5
            if self.regs[VERSION_REG] != -1 or i < 6:
                self.failures += 1
                msg = 'Pwr communication failed at reg %d: %s' % (i, e)
                if self.failures != 5: self.log.info(msg)
                else:
                    self.log.warning(msg)
                    self.failures = 0
                return

        self.lcd_page.text('%3dC   Tmp' % self.regs[TEMP_REG],  0, 0)
        self.lcd_page.text('%5.1fV  In' % self.regs[VIN_REG],   0, 1)
        self.lcd_page.text('%5.1fV Out' % self.regs[VOUT_REG],  0, 2)
        self.lcd_page.text(' %04x  Flg' % self.regs[FLAGS_REG], 0, 3)

        self.lcd_page.text('%5.1fA Mot' % self.regs[MOTOR_REG], 10, 0)
        if self.regs[VERSION_REG] < 0x100:
            self.lcd_page.text('%5.1fA Ld1' % self.regs[LOAD1_REG], 10, 1)
            self.lcd_page.text('%5.1fA Ld2' % self.regs[LOAD2_REG], 10, 2)
        self.lcd_page.text('%5.1fA Vdd' % self.regs[VDD_REG],   10, 3)

        if len(update): self.ctrl.state.update(update)

        self.failures = 0
