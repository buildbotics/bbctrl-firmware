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

import fcntl
import os
import select
import logging

from inevent.Constants import *
from inevent import ioctl
from inevent.AbsAxisScaling import AbsAxisScaling
from inevent import Event


EVIOCGRAB = ioctl._IOW(ord('E'), 0x90, 'i') # Grab/Release device



class EventStream:
  def __init__(self, devIndex, devType, devName, log = None):
    self.devIndex = devIndex
    self.devType = devType
    self.devName = devName
    self.log = log if log else logging.getLogger('inevent')

    self.absScale = [None] * ABS_MAX
    self.abs = [0.0] * ABS_MAX
    self.rel = [0.0] * REL_MAX
    self.fd = os.open('/dev/input/event%u' % devIndex, os.O_RDWR)
    fcntl.ioctl(self.fd, EVIOCGRAB, 1) # Grab

    if devType == 'js':
      for axis in range(6):
        self.absScale[axis] = AbsAxisScaling(self, axis)
        self.log.info('Axis %s %s' % (axis, self.absScale[axis]))


  def __iter__(self): return self


  def __next__(self):
    if select.select([self.fd], [], [], 0)[0]:
      event = self.read()
      axis  = event.code
      value = event.value

      if event.type == EV_REL: self.rel[axis] += value
      if event.type == EV_ABS:
        if self.absScale[axis]: value = self.absScale[axis](value)
        self.abs[axis] = value

      return event

    raise StopIteration


  def read(self):
    try:
      data = os.read(self.fd, Event.size)
      if data: return Event.Event(self, data)

    except Exception as e:
      self.log.info('Reading event: %s' % e)


  def __enter__(self): return self


  def release(self):
    try:
      fcntl.ioctl(self.fd, EVIOCGRAB, 0) # Release
      os.close(self.fd)

    except: pass


  def __exit__(self, type, value, traceback): self.release()
