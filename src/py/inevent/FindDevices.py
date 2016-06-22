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

import re
import logging
from inevent.Constants import *

log = logging.getLogger('inevent')


def test_bit(nlst, b):
  index = b / 32
  bit = b % 32
  return index < len(nlst) and nlst[index] & (1 << bit)


def EvToStr(events):
  s = []

  if test_bit(events, EV_SYN):       s.append("EV_SYN")
  if test_bit(events, EV_KEY):       s.append("EV_KEY")
  if test_bit(events, EV_REL):       s.append("EV_REL")
  if test_bit(events, EV_ABS):       s.append("EV_ABS")
  if test_bit(events, EV_MSC):       s.append("EV_MSC")
  if test_bit(events, EV_LED):       s.append("EV_LED")
  if test_bit(events, EV_SND):       s.append("EV_SND")
  if test_bit(events, EV_REP):       s.append("EV_REP")
  if test_bit(events, EV_FF):        s.append("EV_FF" )
  if test_bit(events, EV_PWR):       s.append("EV_PWR")
  if test_bit(events, EV_FF_STATUS): s.append("EV_FF_STATUS")

  return s


class DeviceCapabilities(object):
  def __init__(self, firstLine, filehandle):
    self.EV_SYNevents = []
    self.EV_KEYevents = []
    self.EV_RELevents = []
    self.EV_ABSevents = []
    self.EV_MSCevents = []
    self.EV_LEDevents = []
    self.EV_SNDevents = []
    self.EV_REPevents = []
    self.EV_FFevents = []
    self.EV_PWRevents = []
    self.EV_FF_STATUSevents = []
    self.eventTypes = []

    match = re.search(".*Bus=([0-9A-Fa-f]+).*Vendor=([0-9A-Fa-f]+).*"
                      "Product=([0-9A-Fa-f]+).*Version=([0-9A-Fa-f]+).*",
                      firstLine)

    if not match:
      log.warning("Do not understand device ID:", line)
      self.bus = 0
      self.vendor = 0
      self.product = 0
      self.version = 0

    else:
      self.bus = int(match.group(1), base = 16)
      self.vendor = int(match.group(2), base = 16)
      self.product = int(match.group(3), base = 16)
      self.version = int(match.group(4), base = 16)

    for line in filehandle:
      if not line.strip(): break

      if line[0] == "N":
        match = re.search('Name="([^"]+)"', line)
        if match: self.name = match.group(1)
        else: self.name = "UNKNOWN"

      elif line[0] == "P":
        match = re.search('Phys=(.+)', line)
        if match: self.phys = match.group(1)
        else: self.phys = "UNKNOWN"

      elif line[0] == "S":
          match = re.search('Sysfs=(.+)', line)
          if match: self.sysfs = match.group(1)
          else: self.sysfs = "UNKNOWN"

      elif line[0] == "U":
        match = re.search('Uniq=(.*)', line)
        if match: self.uniq = match.group(1)
        else: self.uniq = "UNKNOWN"

      elif line[0] == "H":
        match = re.search('Handlers=(.+)', line)
        if match: self.handlers = match.group(1).split()
        else: self.handlers = []

      elif line[:5] == "B: EV":
        eventsNums = [int(x, base = 16) for x in line[6:].split()]
        eventsNums.reverse()
        self.eventTypes = eventsNums

      elif line[:6] == "B: KEY":
        eventsNums = [int(x, base = 16) for x in line[7:].split()]
        eventsNums.reverse()
        self.EV_KEYevents = eventsNums

      elif line[:6] == "B: ABS":
        eventsNums = [int(x, base = 16) for x in line[7:].split()]
        eventsNums.reverse()
        self.EV_ABSevents = eventsNums

      elif line[:6] == "B: MSC":
        eventsNums = [int(x, base = 16) for x in line[7:].split()]
        eventsNums.reverse()
        self.EV_MSCevents = eventsNums

      elif line[:6] == "B: REL":
        eventsNums = [int(x, base = 16) for x in line[7:].split()]
        eventsNums.reverse()
        self.EV_RELevents = eventsNums

      elif line[:6] == "B: LED":
        eventsNums = [int(x, base = 16) for x in line[7:].split()]
        eventsNums.reverse()
        self.EV_LEDevents = eventsNums

    for handler in self.handlers:
      if handler[:5] == "event": self.eventIndex = int(handler[5:])

    self.isMouse = False
    self.isKeyboard = False
    self.isJoystick = False


  def doesProduce(self, eventType, eventCode):
    return test_bit(self.eventTypes, eventType) and (
      (eventType == EV_SYN and test_bit(self.EV_SYNevents, eventCode)) or
      (eventType == EV_KEY and test_bit(self.EV_KEYevents, eventCode)) or
      (eventType == EV_REL and test_bit(self.EV_RELevents, eventCode)) or
      (eventType == EV_ABS and test_bit(self.EV_ABSevents, eventCode)) or
      (eventType == EV_MSC and test_bit(self.EV_MSCevents, eventCode)) or
      (eventType == EV_LED and test_bit(self.EV_LEDevents, eventCode)) or
      (eventType == EV_SND and test_bit(self.EV_SNDevents, eventCode)) or
      (eventType == EV_REP and test_bit(self.EV_REPevents, eventCode)) or
      (eventType == EV_FF  and test_bit(self.EV_FFevents,  eventCode)) or
      (eventType == EV_PWR and test_bit(self.EV_PWRevents, eventCode)) or
      (eventType == EV_FF_STATUS and
       test_bit(self.EV_FF_STATUSevents, eventCode)))


  def __str__(self):
    return (
      "%s\n"
      "Bus: %s Vendor: %s Product: %s Version: %s\n"
      "Phys: %s\n"
      "Sysfs: %s\n"
      "Uniq: %s\n"
      "Handlers: %s Event Index: %s\n"
      "Keyboard: %s Mouse: %s Joystick: %s\n"
      "Events: %s" % (
        self.name, self.bus. self.vendor, self.product, self.version, self.phys,
        self.sysfs, self.uniq, self.handlers, self.eventIndex, self.isKeyboard,
        self.isMouse, self.isJoystick, EvToStr(self.eventTypes)))


deviceCapabilities = []


def get_devices(filename = "/proc/bus/input/devices"):
  global deviceCapabilities

  with open("/proc/bus/input/devices", "r") as filehandle:
    for line in filehandle:
      if line[0] == "I":
        deviceCapabilities.append(DeviceCapabilities(line, filehandle))

  return deviceCapabilities


def print_devices():
  devs = get_devices()

  for dev in devs:
    print(str(dev))
    print("   ABS: {}"
          .format([x for x in range(64) if test_bit(dev.EV_ABSevents, x)]))
    print("   REL: {}"
          .format([x for x in range(64) if test_bit(dev.EV_RELevents, x)]))
    print("   MSC: {}"
          .format([x for x in range(64) if test_bit(dev.EV_MSCevents, x)]))
    print("   KEY: {}"
          .format([x for x in range(512) if test_bit(dev.EV_KEYevents, x)]))
    print("   LED: {}"
          .format([x for x in range(64) if test_bit(dev.EV_LEDevents, x)]))
    print()
