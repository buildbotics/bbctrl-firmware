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

import struct

from inevent import Format
from inevent.Constants import *



class Event(object):
  """
  A single event from the linux input event system.

  Events are tuples: (Time, Type, Code, Value)
  In addition we remember the stream it came from.

  Externally, only the unhandled event handler gets passed the whole event,
  but the SYN handler gets the code and value. (Also the keyboard handler, but
  those are renamed to key and value.)

  This class is responsible for converting the Linux input event structure into
  one of these objects and back again.
  """
  def __init__(self, stream, time = None, type = None, code = None,
               value = None):
    """
    Create a new event.

    Generally all but the stream parameter are left out; we will want to
    populate the object from a Linux input event using decode.
    """
    self.stream = stream
    self.time = time
    self.type = type
    self.code = code
    self.value = value


  def get_type_name(self):
    if self.type not in ev_type_name: return '0x%x' % self.type
    return ev_type_name[self.type]


  def get_source(self):
    return "%s[%d]" % (self.stream.devType, self.stream.devIndex)


  def __str__(self):
    """
    Uses the stream to give the device type and whether it is currently grabbed.
    """
    grabbed = "grabbed" if self.stream.grabbed else "ungrabbed"

    return "Event %s %s @%f: %s 0x%x=0x%x" % (
      self.get_source(), grabbed, self.time, self.get_type_name(), self.code,
      self.value)


  def __repr__(self):
    return "Event(%s, %f, 0x%x, 0x%x, 0x%x)" % (
      repr(self.stream), self.time, self.type, self.code, self.value)


  def encode(self):
    """
    Encode this event into a Linux input event structure.

    The output is packed into a string. It is unlikely that this function
    will be required, but it might as well be here.
    """
    tint = long(self.time)
    tfrac = long((self.time - tint) * 1000000)

    return \
        struct.pack(Format.Event, tsec, tfrac, self.type, self.code, self.value)


  def decode(self, s):
    """
    Decode a Linux input event into the fields of this object.

    Arguments:
      *s*
        A binary structure packed into a string.
    """
    tsec, tfrac, self.type, self.code, self.value = \
        struct.unpack(Format.Event, s)

    self.time = tsec + tfrac / 1000000.0
