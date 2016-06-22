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


class EventState:
  def __init__(self):
    self.abs = [0.0] * ABS_MAX
    self.rel = [0.0] * REL_MAX


  def __str__(self):
    return ("({:6.3f}, {:6.3f}, {:6.3f}) ".format(*self.get_joystick3d()) +
            "({:6.3f}, {:6.3f}, {:6.3f}) ".format(*self.get_joystickR3d()) +
            "({:2.0f}, {:2.0f}) ".format(*self.get_hat()) +
            "({:d}, {:d}) ".format(*self.get_mouse()) +
            "({:d}, {:d})".format(*self.get_wheel()))


  def get_joystick(self):
    """
    Returns the x,y coordinates for a joystick or left gamepad analogue stick.

    The values are returned as a tuple. All values are -1.0 to +1.0 with
    0.0 being centred.
    """
    return self.abs[ABS_X], self.abs[ABS_Y]


  def get_joystick3d(self):
    """
    Returns the x,y,z coordinates for a joystick or left gamepad analogue stick

    The values are returned as a tuple. All values are -1.0 to +1.0 with
    0.0 being centred.
    """
    return self.abs[ABS_X], self.abs[ABS_Y], self.abs[ABS_Z]


  def get_joystickR(self):
    """
    Returns the x,y coordinates for a right gamepad analogue stick.

    The values are returned as a tuple. For some odd reason, the gamepad
    returns values in the Z axes of both joysticks, with y being the first.

    All values are -1.0 to +1.0 with 0.0 being centred.
    """
    return self.abs[ABS_RZ], self.abs[ABS_Z]


  def get_joystickR3d(self):
    """
    Returns the x,y,z coordinates for a 2nd joystick control

    The values are returned as a tuple. All values are -1.0 to +1.0 with
    0.0 being centred.
    """
    return self.abs[ABS_RX], self.abs[ABS_RY], self.abs[ABS_RZ]


  def get_hat(self):
    """
    Returns the x,y coordinates for a joystick hat or gamepad direction pad

    The values are returned as a tuple.  All values are -1.0 to +1.0 with
    0.0 being centred.
    """
    return self.abs[ABS_HAT0X], self.abs[ABS_HAT0Y]


  def get_mouse(self):
    return self.rel[REL_X], self.rel[REL_Y]


  def get_wheel(self):
    return self.rel[REL_WHEEL], self.rel[REL_HWHEEL]


  def get_mouse_movement(self):
    """
    Returns the accumulated REL (mouse or other relative device) movements
    since the last call.

    The returned value is a tuple: (X, Y, WHEEL, H-WHEEL)
    """
    ret = self.get_mouse() + self.get_wheel()

    self.rel[REL_X] = self.rel[REL_Y] = 0
    self.rel[REL_WHEEL] = self.rel[REL_HWHEEL] = 0

    return ret
