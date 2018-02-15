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

import array
import fcntl
import struct
from inevent import ioctl


def EVIOCGABS(axis):
  return ioctl._IOR(ord('E'), 0x40 + axis, "ffffff") # get abs value/limits



class AbsAxisScaling(object):
  """
  Fetches and implements the EV_ABS axis scaling.

  The constructor fetches the scaling values from the given stream for the
  given axis using an ioctl.

  There is a scale method, which scales a given value to the range -1..+1.
  """

  def __init__(self, stream, axis):
    """
    Fetch the scale values for this stream and fill in the instance
    variables accordingly.
    """
    s = array.array("i", [1, 2, 3, 4, 5, 6])
    try:
      fcntl.ioctl(stream.filehandle, EVIOCGABS(axis), s)

    except IOError:
      self.value = self.minimum = self.maximum = self.fuzz = self.flat = \
          self.resolution = 1

    else:
      self.value, self.minimum, self.maximum, self.fuzz, self.flat, \
          self.resolution = struct.unpack("iiiiii", s)


  def __str__(self):
    return "Value {0} Min {1}, Max {2}, Fuzz {3}, Flat {4}, Res {5}".format(
      self.value, self.minimum, self.maximum, self.fuzz, self.flat,
      self.resolution)


  def scale(self, value):
    """
    scales the given value into the range -1..+1
    """
    return (float(value) - float(self.minimum)) / \
        float(self.maximum - self.minimum) * 2.0 - 1.0
