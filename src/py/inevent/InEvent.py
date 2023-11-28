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

import re
import select
import errno
import functools
import logging

from inevent.Constants import *
from inevent.EventStream import EventStream


class InEvent:
  def __init__(
      self, ioloop, udevev, cb, types = 'kbd mouse js'.split(), log = None):
    self.ioloop  = ioloop
    self.udevev  = udevev
    self.cb      = cb
    self.streams = []
    self.types   = types
    self.log     = log if log else logging.getLogger('inevent')

    for index, type, name in self.find_devices(types):
      self.add(index, type, name)

    udevev.add_handler(self._input_event_handler, 'input')


  def get_dev(self, index):
    return self.udevev.dev_by_name('input', 'event%s' % index)


  def get_dev_name(self, index):
    try:
      dev = self.get_dev(index)
      return dev.parent.attributes.asstring('name').strip()

    except Exception as e:
      self.log.error(e)


  def find_devices(self, types):
    '''Find the event indices of all devices of the specified types.

    A type is a string on the handlers line of /proc/bus/input/devices.
    Keyboards use "kbd", mice use "mouse" and joysticks (and gamepads) use "js".

    Return a list of integer indexes N, where /dev/input/eventN is the event
    stream for each device.
    '''
    with open('/proc/bus/input/devices', 'r') as f:
      for line in f:
        if line[0] == 'H':
          for type in types:
            if type in line:
              match = re.search('event([0-9]+)', line)
              index = match and match.group(1)
              if index: yield int(index), type, self.get_dev_name(index)


  def _input_event_handler(self, action, device):
    match = re.search(r'/dev/input/event([0-9]+)', str(device.device_node))
    devIndex = match and match.group(1)
    if not devIndex: return
    devIndex = int(devIndex)

    if action == 'add':
      for index, devType, devName in self.find_devices(self.types):
        if index == devIndex:
          self.add(devIndex, devType, devName)
          break

    if action == 'remove': self.remove(devIndex)


  def add(self, devIndex, devType, devName):
    try:
      stream = EventStream(devIndex, devType, devName, self.log)
      self.streams.append(stream)

      def io_handler(fd, events):
        for event in stream: self.cb(event)

      self.ioloop.add_handler(stream.fd, io_handler, self.ioloop.READ)
      self.log.info('Added %s[%d] %s', devType, devIndex, devName)

    except OSError as e:
      self.log.warning('Failed to add %s[%d]: %s', devType, devIndex, e)


  def remove(self, devIndex):
    for stream in self.streams:
      if stream.devIndex == devIndex:
        self.streams.remove(stream)
        self.ioloop.remove_handler(stream.fd)
        stream.release()
        self.cb.clear()

        self.log.info('Removed %s[%d]', stream.devType, devIndex)


  def release(self):
    for s in self.streams: s.release()
