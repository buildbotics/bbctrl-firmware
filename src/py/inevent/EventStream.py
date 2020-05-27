################################################################################
#                                                                              #
#                 This file is part of the Buildbotics firmware.               #
#                                                                              #
#        Copyright (c) 2015 - 2020, Buildbotics LLC, All rights reserved.      #
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
# Copyright (c) 2016, Joseph Coffland, Cauldron Development LLC.
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
from inevent.EventState import EventState


log = logging.getLogger('inevent')

EVIOCGRAB = ioctl._IOW(ord('E'), 0x90, "i") # Grab/Release device



class EventStream(object):
  """
  encapsulates the event* file handling

  Each device is represented by a file in /dev/input called eventN, where N is
  a small number. (Actually, a keybaord can be represented by two such files.)
  Instances of this class open one of these files and provide means to read
  events from them.

  Class methods also exist to read from multiple files simultaneously, and
  also to grab and ungrab all instances of a given type.
  """
  axisX = 0
  axisY = 1
  axisZ = 2
  axisRX = 3
  axisRY = 4
  axisRZ = 5
  axisHat0X = 6
  axisHat0Y = 7
  axisHat1X = 8
  axisHat1Y = 9
  axisHat2X = 10
  axisHat2Y = 11
  axisHat3X = 12
  axisHat3Y = 13
  axisThrottle = 14
  axisRudder = 15
  axisWheel = 16
  axisGas = 17
  axisBrake = 18
  axisPressure = 19
  axisDistance = 20
  axisTiltX = 21
  axisTiltY = 22
  axisToolWidth = 23
  numAxes = 24

  axisToEvent = [
    ABS_X, ABS_Y, ABS_Z, ABS_RX, ABS_RY, ABS_RZ, ABS_HAT0X, ABS_HAT0Y,
    ABS_HAT1X, ABS_HAT1Y, ABS_HAT2X, ABS_HAT2Y, ABS_HAT3X, ABS_HAT3Y,
    ABS_THROTTLE, ABS_RUDDER, ABS_WHEEL, ABS_GAS, ABS_BRAKE, ABS_PRESSURE,
    ABS_DISTANCE, ABS_TILT_X, ABS_TILT_Y, ABS_TOOL_WIDTH]


  def __init__(self, devIndex, devType, devName):
    """
    Opens the given /dev/input/event file and grabs it.

    Also adds it to a class-global list of all existing streams.
    """
    self.devIndex = devIndex
    self.devType = devType
    self.devName = devName
    self.filename = "/dev/input/event" + str(devIndex)
    self.filehandle = os.open(self.filename, os.O_RDWR)
    self.state = EventState()
    self.grab(True)
    self.absInfo = [None] * ABS_MAX

    if devType == "js":
      for axis in range(ABS_MAX):
        self.absInfo[axis] = AbsAxisScaling(self, axis)


  def scale(self, axis, value):
    """
    Scale the given value according to the given axis.

    acquire_abs_info must have been previously called to acquire the data to
    do the scaling.
    """
    assert axis < ABS_MAX, "Axis number out of range"

    if self.absInfo[axis]: return self.absInfo[axis].scale(value)
    else: return value


  def grab(self, grab = True):
    """
    Grab (or release) exclusive access to all devices of the given type.

    The devices are grabbed if grab is True and released if grab is False.

    All devices are grabbed to begin with. We might want to ungrab the
    keyboard for example to use it for text entry. While not grabbed, all
    key-down and key-hold events are filtered out.
    """
    fcntl.ioctl(self.filehandle, EVIOCGRAB, 1 if grab else 0)
    self.grabbed = grab


  def __iter__(self):
    """s
    Required to make this class an iterator
    """
    return self


  def next(self): return self.__next__()


  def __next__(self):
    """
    Returns the next waiting event.

    If no event is waiting, returns None.
    """
    ready = select.select([self.filehandle], [], [], 0)[0]
    if ready: return self.read()


  def read(self):
    """
    Read and return the next waiting event.
    """
    try:
      s = os.read(self.filehandle, Event.size)
      if s:
        event = Event.Event(self)
        event.decode(s)
        return event

    except Exception as e:
      log.info('Reading event: %s' % e)


  def __enter__(self): return self


  def release(self):
    "Ungrabs the file and closes it."

    try:
      self.grab(False)
      os.close(self.filehandle)

    except:
      pass


  def __exit__(self, type, value, traceback):
    "Ungrabs the file and closes it."

    self.release()
