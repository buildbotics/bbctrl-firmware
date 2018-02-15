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

from inevent.Constants import *
from inevent.EventStream import EventStream


class EventHandler(object):
  """
  A class to handle events.

  Four types of events are handled: REL (mouse movement), KEY (keybaord keys and
  other device buttons), ABS (joysticks and gamepad analogue sticks) and SYN
  (delimits simultaneous events such as mouse movements)
  """
  def __init__(self):
    self.buttons = dict()


  def event(self, event, handler, name):
    """
    Handles the given event.

    If the event is passed to a handler or otherwise handled then returns None,
    else returns the event. All handlers are optional.

    All key events are handled by putting them in the self.buttons dict, and
    optionally by calling the supplied handler.

    REL X, Y and wheel V and H events are all accumulated internally and
    also optionally passed to the supplied handler. All these events are
    handled.

    ABS X, Y, Z, RX, RY, RZ, Hat0X, Hat0Y are all accumulated internally and
    also optionally passed to the supplied handler. Other ABS events are not
    handled.

    All SYN events are passed to the supplied handler.

    There are several ABS events that we do not handle. In particular:
    THROTTLE, RUDDER, WHEEL, GAS, BRAKE, HAT1, HAT2, HAT3, PRESSURE,
    DISTANCE, TILT, TOOL_WIDTH. Implementing these is left as an exercise
    for the interested reader.

    Likewise, since one handler is handling all events for all devices, we
    may get the situation where two devices return the same button. The only
    way to handle that would seem to be to have a key dict for every device,
    which seems needlessly profligate for a situation that may never arise.
    """

    state = event.stream.state

    if event.type == EV_KEY: self.buttons[event.code] = event.value
    elif event.type == EV_REL: state.rel[event.code] += event.value
    elif event.type == EV_ABS:
      state.abs[event.code] = event.stream.scale(event.code, event.value)

    if handler: handler.event(event, state, name)


  def key_state(self, code):
    """
    Returns the last event value for the given key code.

    Key names can be converted to key codes using codeOf[str].
    If the key is pressed the returned value will be 1 (pressed) or 2 (held).
    If the key is not pressed, the returned value will be 0.
    """
    return self.buttons.get(code, 0)


  def clear_key(self, code):
    """
    Clears the event value for the given key code.

    Key names can be converted to key codes using codeOf[str].
    This emulates a key-up but does not generate any events.
    """
    self.buttons[code] = 0


  def get_keys(self):
    """
    Returns the first of whichever keys have been pressed.

    Key names can be converted to key codes using codeOf[str].
    This emulates a key-up but does not generate any events.
    """
    k_list = []

    for k in self.buttons:
      if self.buttons[k] != 0: k_list.append(k)

    return k_list
