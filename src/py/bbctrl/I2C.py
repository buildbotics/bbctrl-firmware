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

import errno

try:
    try:
        import smbus
    except:
        import smbus2 as smbus
except:
    smbus = None


class I2C(object):
    def __init__(self, port, disabled):
        self.port = port
        self.i2c_bus = None
        self.disabled = disabled or smbus is None


    def connect(self):
        if self.disabled: return
        if self.i2c_bus is None:
            try:
                self.i2c_bus = smbus.SMBus(self.port)

            except OSError as e:
                self.i2c_bus = None
                if e.errno == errno.ENOENT: self.disabled = True
                else: raise type(e)('I2C failed to open device: %s' % e)


    def read_word(self, addr):
        self.connect()
        if self.disabled: return

        try:
            return self.i2c_bus.read_word_data(addr, 0)

        except IOError as e:
            self.i2c_bus.close()
            self.i2c_bus = None
            raise type(e)('I2C read word failed: %s' % e)


    def write(self, addr, cmd, byte = None, word = None, block = None):
        self.connect()
        if self.disabled: return

        try:
            if byte is not None:
                self.i2c_bus.write_byte_data(addr, cmd, byte)

            elif word is not None:
                self.i2c_bus.write_word_data(addr, cmd, word)

            elif block is not None:
                if isinstance(block, str): block = list(map(ord, block))
                self.i2c_bus.write_i2c_block_data(addr, cmd, block)

            else: self.i2c_bus.write_byte(addr, cmd)

        except IOError as e:
            self.i2c_bus.close()
            self.i2c_bus = None
            raise type(e)('I2C write failed: %s' % e)
