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


    def read_word(self, addr, reg, pec = False):
        self.connect()
        if self.disabled: return

        try:
            self.i2c_bus.pec = pec
            return self.i2c_bus.read_word_data(addr, reg)

        except IOError as e:
            self.i2c_bus.close()
            self.i2c_bus = None
            raise type(e)('I2C read word failed: %s' % e)


    def write(self, addr, cmd, byte = None, word = None, block = None,
              pec = False):
        self.connect()
        if self.disabled: return

        self.i2c_bus.pec = pec

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
