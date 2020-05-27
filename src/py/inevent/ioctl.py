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
#
# IOCTL macros
#
# ioctl command encoding: 32 bits total, command in lower 16 bits,
# size of the parameter structure in the lower 14 bits of the
# upper 16 bits.
#
# Encoding the size of the parameter structure in the ioctl request
# is useful for catching programs compiled with old versions
# and to avoid overwriting user space outside the user buffer area.
# The highest 2 bits are reserved for indicating the ``access mode''.
# NOTE: This limits the max parameter size to 16kB - 1
#
# The following is for compatibility across the various Linux
# platforms.  The generic ioctl numbering scheme doesn't really enforce
# a type field.  De facto, however, the top 8 bits of the lower 16
# bits are indeed used as a type field, so we might just as well make
# this explicit here.

import struct


sizeof = struct.calcsize

_IOC_NRBITS    = 8
_IOC_TYPEBITS  = 8
_IOC_SIZEBITS  = 14
_IOC_DIRBITS   = 2

_IOC_NRMASK    = (1 << _IOC_NRBITS)   - 1
_IOC_TYPEMASK  = (1 << _IOC_TYPEBITS) - 1
_IOC_SIZEMASK  = (1 << _IOC_SIZEBITS) - 1
_IOC_DIRMASK   = (1 << _IOC_DIRBITS)  - 1

_IOC_NRSHIFT   = 0
_IOC_TYPESHIFT = _IOC_NRSHIFT   + _IOC_NRBITS
_IOC_SIZESHIFT = _IOC_TYPESHIFT + _IOC_TYPEBITS
_IOC_DIRSHIFT  = _IOC_SIZESHIFT + _IOC_SIZEBITS

_IOC_NONE      = 0
_IOC_WRITE     = 1
_IOC_READ      = 2
_IOC_RW        = _IOC_READ | _IOC_WRITE


def _IOC(dir, type, nr, size):
  return int(
    (dir  << _IOC_DIRSHIFT)  |
    (type << _IOC_TYPESHIFT) |
    (nr   << _IOC_NRSHIFT)   |
    (size << _IOC_SIZESHIFT))

# encode ioctl numbers
def _IO(type, nr):            return _IOC(_IOC_NONE,  type, nr, 0)
def _IOR(type, nr, fmt):      return _IOC(_IOC_READ,  type, nr, sizeof(fmt))
def _IOW(type, nr, fmt):      return _IOC(_IOC_WRITE, type, nr, sizeof(fmt))
def _IOWR(type, nr, fmt):     return _IOC(_IOC_RW,    type, nr, sizeof(fmt))
def _IOR_BAD(type, nr, fmt):  return _IOC(_IOC_READ,  type, nr, sizeof(fmt))
def _IOW_BAD(type, nr, fmt):  return _IOC(_IOC_WRITE, type, nr, sizeof(fmt))
def _IOWR_BAD(type, nr, fmt): return _IOC(_IOC_RW,    type, nr, sizeof(fmt))

# decode ioctl numbers
def _IOC_DIR(nr):  return (nr >> _IOC_DIRSHIFT)  & _IOC_DIRMASK
def _IOC_TYPE(nr): return (nr >> _IOC_TYPESHIFT) & _IOC_TYPEMASK
def _IOC_NR(nr):   return (nr >> _IOC_NRSHIFT)   & _IOC_NRMASK
def _IOC_SIZE(nr): return (nr >> _IOC_SIZESHIFT) & _IOC_SIZEMASK

# for drivers/sound files
IOC_IN        = _IOC_WRITE    << _IOC_DIRSHIFT
IOC_OUT       = _IOC_READ     << _IOC_DIRSHIFT
IOC_INOUT     = _IOC_RW       << _IOC_DIRSHIFT
IOCSIZE_MASK  = _IOC_SIZEMASK << _IOC_SIZESHIFT
IOCSIZE_SHIFT = _IOC_SIZESHIFT

