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

import pyudev
import re
import select
import errno
import functools
import logging

from inevent.EventHandler import EventHandler
from inevent import Keys
from inevent.Constants import *
from inevent.EventStream import EventStream


log = logging.getLogger('inevent')

_KEYS = (k for k in vars(Keys) if not k.startswith('_'))
KEY_CODE = dict((k, getattr(Keys, k)) for k in _KEYS)
CODE_KEY = {}
for v in KEY_CODE: CODE_KEY[KEY_CODE[v]] = v


def key_to_code(key):
  return KEY_CODE.get(str(key), -1) \
      if isinstance(key, str) else key


def code_to_key(code): return CODE_KEY.get(code, '')


def find_devices(types):
  """Finds the event indices of all devices of the specified types.

  A type is a string on the handlers line of /proc/bus/input/devices.
  Keyboards use "kbd", mice use "mouse" and joysticks (and gamepads) use "js".

  Returns a list of integer indexes N, where /dev/input/eventN is the event
  stream for each device.

  If butNot is given it holds a list of tuples which the returned values should
  not match.

  All devices of each type are returned; if you have two mice, they will both
  be used.
  """
  with open("/proc/bus/input/devices", "r") as filehandle:
    for line in filehandle:
      if line[0] == "H":
        for type in types:
          if type in line:
            match = re.search("event([0-9]+)", line)
            index = match and match.group(1)
            if index: yield int(index), type
            break



class InEvent(object):
  """Encapsulates the entire InEvent subsystem.

  This is generally all you need to import.

  On instantiation, we open all devices that are keyboards, mice or joysticks.
  That means we might have two of one sort of another, and that might be a
  problem, but it would be rather rare.

  There are several ABS (joystick, touch) events that we do not handle,
  specifically THROTTLE, RUDDER, WHEEL, GAS, BRAKE, HAT1, HAT2, HAT3, PRESSURE,
  DISTANCE, TILT, TOOL_WIDTH. Implementing these is left as an exercise
  for the interested reader. Similarly, we make no attempt to handle
  multi-touch.

  Handlers can be supplied, in which case they are called for each event, but
  it isn't necessary; API exists for all the events.

  The handler signature is:

    def handler_func(event, state)

  where:
    event:
      An Event object describing the event.

    state:
      An EventState object describing the current state.

  Use key_to_code() to convert from the name of a key to its code,
  and code_to_key() to convert a code to a name.

  The keys are listed in inevent.Constants.py or /usr/include/linux/input.h
  Note that the key names refer to a US keyboard.
  """
  def __init__(self, ioloop, cb, types = ["kbd", "mouse", "js"]):
    self.ioloop = ioloop
    self.cb = cb
    self.streams = []
    self.handler = EventHandler()
    self.types = types

    devs = list(find_devices(types))
    for index, type in devs:
      self.add_stream(index, type)

    self.udevCtx = pyudev.Context()
    self.udevMon = pyudev.Monitor.from_netlink(self.udevCtx)
    self.udevMon.filter_by(subsystem = 'input')
    self.udevMon.start()
    ioloop.add_handler(self.udevMon.fileno(), self.udev_handler, ioloop.READ)


  def process_udev_event(self):
    action, device = self.udevMon.receive_device()
    if device is None: return

    match = re.search(r"/dev/input/event([0-9]+)", str(device.device_node))
    devIndex = match and match.group(1)
    if not devIndex: return
    devIndex = int(devIndex)

    if action == 'add':
      for index, devType in find_devices(self.types):
        if index == devIndex:
          self.add_stream(devIndex, devType)
          break

    if action == 'remove':
      self.remove_stream(devIndex)


  def stream_handler(self, fd, events):
    for stream in self.streams:
      if stream.filehandle == fd:
        while True:
          event = stream.next()
          if event: self.handler.event(event, self.cb)
          else: break


  def udev_handler(self, fd, events):
    self.process_udev_event()


  def add_stream(self, devIndex, devType):
    try:
      stream = EventStream(devIndex, devType)
      self.streams.append(stream)

      self.ioloop.add_handler(stream.filehandle, self.stream_handler,
                              self.ioloop.READ)

      log.info('Added %s[%d]', devType, devIndex)

    except OSError as e:
      log.warning('Failed to add %s[%d]: %s', devType, devIndex, e)


  def remove_stream(self, devIndex):
    for stream in self.streams:
      if stream.devIndex == devIndex:
        self.streams.remove(stream)
        self.ioloop.remove_handler(stream.filehandle)
        stream.release()

        log.info('Removed %s[%d]', stream.devType, devIndex)


  def key_state(self, key):
    """
    Returns the state of the given key.

    The returned value will be 0 for key-up, or 1 for key-down. This method
    returns a key-held(2) as 1 to aid in using the returned value as a
    movement distance.

    This function accepts either the key code or the string name of the key.
    It would be more efficient to look-up and store the code of
    the key with KEY_CODE[], rather than using the string every time. (Which
    involves a dict look-up keyed with a string for every key_state call, every
    time around the loop.)

    Gamepad keys are:
      Select = BTN_BASE3, Start = BTN_BASE4
      L1 = BTN_TOP       R1 = BTN_BASE
      L2 = BTN_PINKIE    R2 = BTN_BASE2

    The action buttons are:
      BTN_THUMB
      BTN_TRIGGER
      BTN_TOP
      BTN_THUMB2

    Analogue Left Button = BTN_BASE5
    Analogue Right Button = BTN_BASE6

    Some of those may clash with extended mouse buttons, so if you are using
    both at once, you'll see some overlap.

    The direction pad is hat0 (see get_hat)
    """
    return self.handler.key_state(key_to_code(key))


  def clear_key(self, key):
    """
    Clears the state of the given key.

    Emulates a key-up, but does not call any handlers.
    """
    return self.handler.clear_key(key_to_code(key))


  def get_keys(self):
    return [code_to_key(k) for k in self.handler.get_keys()]


  def release(self):
    """
    Ungrabs all streams and closes all files.

    Only do this when you're finished with this object. You can't use it again.
    """
    for s in self.streams: s.release()
