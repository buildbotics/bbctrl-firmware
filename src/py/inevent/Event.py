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

# The inevent Python module was adapted from pi3d.event from the pi3d
# project.
#
# Copyright (c) 2016 - 2023, Joseph Coffland, Cauldron Development LLC.
# Copyright (c) 2015, Tim Skillman.
# Copyright (c) 2015, Paddy Gaunt.
# Copyright (c) 2015, Tom Ritchford.
#
# Permission is hereby granted, free of charge, to any person
# obtaining a copy of this software and associated documentation files
# (the "Software"), to deal in the Software without restriction,
# including without limitation the rights to use, copy, modify, merge,
# publish, distribute, sublicense, and/or sell copies of the Software,
# and to permit persons to whom the Software is furnished to do so,
# subject to the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
# BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
# ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

import struct

from inevent.Constants import *

_format = 'llHHi'
size = struct.calcsize(_format)


def axes_to_string(axes):
    s = ''
    for axis in axes:
        if s: s += ', '
        else: s = '('
        s += '{:6.3f}'.format(axis)
    return s + ')'


class Event:
  def __init__(self, stream, data):
    self.stream = stream

    tsec, tfrac, self.type, self.code, self.value = struct.unpack(_format, data)
    self.time = tsec + tfrac / 1000000.0


  def get_type_name(self):
    if self.type not in ev_type_name: return '0x%x' % self.type
    return ev_type_name[self.type]


  def get_source(self):
    return '%s[%d]' % (self.stream.devType, self.stream.devIndex)


  def __str__(self):
    s = 'Event %s @%f: %s 0x%x=0x%x' % (
      self.get_source(), self.time, self.get_type_name(), self.code, self.value)

    if self.type == EV_ABS:
        abs = self.stream.abs
        s += axes_to_string((abs[ABS_X], abs[ABS_Y], abs[ABS_Z])) + ' ' + \
            axes_to_string((abs[ABS_RX], abs[ABS_RY], abs[ABS_RZ])) + ' ' + \
            '({:2.0f}, {:2.0f}) '.format(abs[ABS_HAT0X], abs[ABS_HAT0Y])

    if self.type == EV_REL:
        rel = self.stream.rel
        s += '({:d}, {:d}) '.format(rel[REL_X], rel[REL_Y]) + \
            '({:d}, {:d})'.format(rel[REL_WHEEL], rel[REL_HWHEEL])

    if self.type == EV_KEY:
        state = 'pressed' if self.value else 'released'
        s += '0x{:x} {}'.format(self.code, state)

    return s


  def __repr__(self):
    return 'Event(%s, %f, 0x%x, 0x%x, 0x%x)' % (
      repr(self.stream), self.time, self.type, self.code, self.value)
