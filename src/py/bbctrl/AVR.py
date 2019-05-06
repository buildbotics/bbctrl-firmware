################################################################################
#                                                                              #
#                This file is part of the Buildbotics firmware.                #
#                                                                              #
#                  Copyright (c) 2015 - 2018, Buildbotics LLC                  #
#                             All rights reserved.                             #
#                                                                              #
#     This file ("the software") is free software: you can redistribute it     #
#     and/or modify it under the terms of the GNU General Public License,      #
#      version 2 as published by the Free Software Foundation. You should      #
#      have received a copy of the GNU General Public License, version 2       #
#     along with the software. If not, see <http://www.gnu.org/licenses/>.     #
#                                                                              #
#     The software is distributed in the hope that it will be useful, but      #
#          WITHOUT ANY WARRANTY; without even the implied warranty of          #
#      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU       #
#               Lesser General Public License for more details.                #
#                                                                              #
#       You should have received a copy of the GNU Lesser General Public       #
#                License along with the software.  If not, see                 #
#                       <http://www.gnu.org/licenses/>.                        #
#                                                                              #
#                For information regarding this software email:                #
#                  "Joseph Coffland" <joseph@buildbotics.com>                  #
#                                                                              #
################################################################################

import serial
import time
import traceback
import ctypes

import bbctrl
import bbctrl.Cmd as Cmd


class serial_struct(ctypes.Structure):
    _fields_ = [
        ('type',            ctypes.c_int),
        ('line',            ctypes.c_int),
        ('port',            ctypes.c_uint),
        ('irq',             ctypes.c_int),
        ('flags',           ctypes.c_int),
        ('xmit_fifo_size',  ctypes.c_int),
        ('custom_divisor',  ctypes.c_int),
        ('baud_base',       ctypes.c_int),
        ('close_delay',     ctypes.c_ushort),
        ('io_type',         ctypes.c_byte),
        ('reserved',        ctypes.c_byte),
        ('hub6',            ctypes.c_int),
        ('closing_wait',    ctypes.c_ushort),
        ('closing_wait2',   ctypes.c_ushort),
        ('iomem_base',      ctypes.c_char_p),
        ('iomem_reg_shift', ctypes.c_ushort),
        ('port_high',       ctypes.c_uint),
        ('iomap_base',      ctypes.c_ulong),
    ]


def serial_set_low_latency(sp):
    import fcntl
    import termios

    ASYNCB_LOW_LATENCY = 13

    ss = serial_struct()
    fcntl.ioctl(sp, termios.TIOCGSERIAL, ss)
    ss.flags |= 1 << ASYNCB_LOW_LATENCY
    fcntl.ioctl(sp, termios.TIOCSSERIAL, ss)


class AVR(object):
    def __init__(self, ctrl):
        self.ctrl = ctrl
        self.log = ctrl.log.get('AVR')

        self.sp = None
        self.i2c_addr = ctrl.args.avr_addr
        self.read_cb = None
        self.write_cb = None


    def close(self): pass


    def _start(self):
        try:
            self.sp = serial.Serial(self.ctrl.args.serial, self.ctrl.args.baud,
                                    rtscts = 1, timeout = 0, write_timeout = 0)
            self.sp.nonblocking()
            serial_set_low_latency(self.sp)

        except Exception as e:
            self.sp = None
            self.log.warning('Failed to open serial port: %s', e)

        if self.sp is not None:
            self.ctrl.ioloop.add_handler(self.sp, self._serial_handler,
                                         self.ctrl.ioloop.READ)


    def set_handlers(self, read_cb, write_cb):
        if self.read_cb is not None or self.write_cb is not None:
            raise Exception('Handler already set')

        self.read_cb = read_cb
        self.write_cb = write_cb
        self._start()


    def enable_write(self, enable):
        if self.sp is None: return

        flags = self.ctrl.ioloop.READ
        if enable: flags |= self.ctrl.ioloop.WRITE
        self.ctrl.ioloop.update_handler(self.sp, flags)


    def _serial_write(self):
        self.write_cb(lambda data: self.sp.write(data))


    def _serial_read(self):
        try:
            data = self.sp.read(self.sp.in_waiting)
            self.read_cb(data)

        except Exception as e:
            self.log.warning('%s: %s', e, data)


    def _serial_handler(self, fd, events):
        try:
            if self.ctrl.ioloop.READ & events: self._serial_read()
            if self.ctrl.ioloop.WRITE & events: self._serial_write()

        except Exception as e:
            self.log.warning('Serial handler error: %s', traceback.format_exc())


    def i2c_command(self, cmd, byte = None, word = None, block = None):
        self.log.info('I2C: %s b=%s w=%s d=%s' % (cmd, byte, word, block))
        retry = 5
        cmd = ord(cmd[0])

        while True:
            try:
                self.ctrl.i2c.write(self.i2c_addr, cmd, byte, word, block)
                break

            except Exception as e:
                retry -= 1

                if retry:
                    self.log.warning('I2C failed, retrying: %s' % e)
                    time.sleep(0.1)
                    continue

                else:
                    self.log.error('I2C failed: %s' % e)
                    raise
