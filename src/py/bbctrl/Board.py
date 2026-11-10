################################################################################
#                                                                              #
#                 This file is part of the Buildbotics firmware.               #
#                                                                              #
#        Copyright (c) 2015 - 2023, Buildbotics LLC, All rights reserved.      #
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


'''
Main board revision handling.

Board revisions differ in what drives the DRV8711 motor driver enable line
(see the bbctrl-pcb netlists):

  Rev 15 and earlier  Main AVR pin 51, active high.  The 2.x AVR firmware
                      calls that pin BUFFER_ENABLE and drives it high from
                      reset, which enables the drivers.  Asserting the 'be'
                      (buffer enable) output pulls it low and disables them.
  Rev 16 and later    The power chip drives it.  AVR pin 51 is free for the
                      buffer enable function, which the host asserts.

Rev 13 and earlier are recognized by their ATtiny1634 power chip, which
answers I2C at 0x60 + register rather than at 0x5f.  Rev 14 and 15 cannot be
told apart from Rev 16 in software, so they must be selected in the settings.
'''

AUTO   = 'auto'
REV_13 = '13 or earlier'
REV_15 = '14 or 15'
REV_16 = '16 or later'

LEGACY_PWR_I2C_ADDR = 0x60
LEGACY_PWR_PROBES   = 3


def resolve(setting: str, has_legacy_pwr_chip: bool) -> str:
    'Board revision to act on, given the setting and what was detected.'
    if setting in (REV_13, REV_15, REV_16): return setting
    return REV_13 if has_legacy_pwr_chip else REV_16


def host_enables_buffers(revision: str) -> bool:
    'Whether the host should assert the AVR buffer enable output.'
    return revision == REV_16


def has_legacy_pwr_chip(i2c) -> bool:
    'Whether an ATtiny1634 power chip (Rev 13 and earlier) answers on I2C.'
    for _ in range(LEGACY_PWR_PROBES):
        try:
            return i2c.read_word(LEGACY_PWR_I2C_ADDR, 0) is not None
        except IOError: pass

    return False
