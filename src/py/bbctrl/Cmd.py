#!/usr/bin/env python3

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

import struct
import base64
import json
import logging

log = logging.getLogger('Cmd')

# Keep this in sync with AVR code command.def
SET          = '$'
SET_SYNC     = '#'
MODBUS_READ  = 'm'
MODBUS_WRITE = 'M'
SEEK         = 's'
SET_AXIS     = 'a'
LINE         = 'l'
INPUT        = 'I'
DWELL        = 'd'
PAUSE        = 'P'
STOP         = 'S'
UNPAUSE      = 'U'
JOG          = 'j'
REPORT       = 'r'
REBOOT       = 'R'
RESUME       = 'c'
ESTOP        = 'E'
CLEAR        = 'C'
FLUSH        = 'F'
DUMP         = 'D'
HELP         = 'h'

SEEK_ACTIVE = 1 << 0
SEEK_ERROR  = 1 << 1


def encode_float(x):
    return base64.b64encode(struct.pack('<f', x))[:-2].decode("utf-8")


def decode_float(s):
    return struct.unpack('<f', base64.b64decode(s + '=='))[0]


def encode_axes(axes):
    data = ''
    for axis in 'xyzabc':
        if axis in axes:
            data += axis + encode_float(axes[axis])

        elif axis.upper() in axes:
            data += axis + encode_float(axes[axis.upper()])

    return data


def set_sync(name, value):
    if isinstance(value, float): return set_float(name, value)
    else: return SET_SYNC + '%s=%s' % (name, value)


def set(name, value):
    if isinstance(value, float): return set_float(name, value)
    else: return SET + '%s=%s' % (name, value)


def set_float(name, value):
    return SET_SYNC + '%s=:%s' % (name, encode_float(value))


def modbus_read(addr): return MODBUS_READ + '%d' % addr
def modbus_write(addr, value): return MODBUS_WRITE + '%d=%d' % (addr, value)
def set_axis(axis, position): return SET_AXIS + axis + encode_float(position)


def line(target, exitVel, maxAccel, maxJerk, times):
    cmd = LINE

    cmd += encode_float(exitVel)
    cmd += encode_float(maxAccel)
    cmd += encode_float(maxJerk)
    cmd += encode_axes(target)

    # S-Curve time parameters
    for i in range(7):
        if times[i]:
            cmd += str(i) + encode_float(times[i] / 60000) # to mins

    return cmd


def speed(speed): return '#s=:' + encode_float(speed)


def input(port, mode, timeout):
    type, index, m = 'd', 0, 0

    # Analog/digital & port index
    if port == 'digital-in-0': type, index = 'd', 0
    if port == 'digital-in-1': type, index = 'd', 1
    if port == 'digital-in-2': type, index = 'd', 2
    if port == 'digital-in-3': type, index = 'd', 3
    if port == 'analog-in-0':  type, index = 'a', 0
    if port == 'analog-in-1':  type, index = 'a', 1
    if port == 'analog-in-2':  type, index = 'a', 2
    if port == 'analog-in-3':  type, index = 'a', 3

    # Mode
    if mode == 'immediate': m = 0
    if mode == 'rise':      m = 1
    if mode == 'fall':      m = 2
    if mode == 'high':      m = 3
    if mode == 'low':       m = 4

    return '%s%s%d%d%s' % (INPUT, type, index, m, encode_float(timeout))


def output(port, value):
    if port == 'mist':  return '#1oa=' + ('1' if value else '0')
    if port == 'flood': return '#2oa=' + ('1' if value else '0')
    raise Exception('Unsupported output "%s"' % port)


def dwell(seconds): return DWELL + encode_float(seconds)


def pause(type):
    if type == 'program': type = 1
    elif type == 'optional': type = 2
    elif type == 'pallet-change': type = 1
    else: raise Exception('Unknown pause type "%s"' % type)

    return '%s%d' % (PAUSE, type)


def jog(axes): return 'j' + encode_axes(axes)


def seek(switch, active, error):
    cmd = SEEK

    if switch == 'probe': cmd += '1'
    elif switch == 'x-min': cmd += '2'
    elif switch == 'x-max': cmd += '3'
    elif switch == 'y-min': cmd += '4'
    elif switch == 'y-max': cmd += '5'
    elif switch == 'z-min': cmd += '6'
    elif switch == 'z-max': cmd += '7'
    elif switch == 'a-min': cmd += '8'
    elif switch == 'a-max': cmd += '9'
    else: raise Exception('Unsupported switch "%s"' % switch)

    flags = 0
    if active: flags |= SEEK_ACTIVE
    if error:  flags |= SEEK_ERROR
    cmd += chr(flags + ord('0'))

    return cmd


def decode_command(cmd):
    if not len(cmd): return

    data = {}

    if cmd[0] == SET or cmd[0] == SET_SYNC:
        data['type'] = 'set'
        if cmd[0] == SET_SYNC: data['sync'] = True

        equal = cmd.find('=')
        data['name'] = cmd[1:equal]

        value = cmd[equal + 1:]

        if value.lower() == 'true': value = True
        elif value.lower() == 'false': value = False
        elif value.find('.') == -1: data['value'] = int(value)
        else: data['value'] = float(value)

    elif cmd[0] == JOG:
        data['type'] = 'jog'

        cmd = cmd[1:]
        while len(cmd):
            name = cmd[0]
            value = decode_float(cmd[1:7])
            cmd = cmd[7:]

            if name in 'xyzabcuvw': data[name] = value


    elif cmd[0] == SEEK:
        data['type'] = 'seek'

        data['port'] = int(cmd[2], 16)
        flags = int(cmd[2], 16)

        data['active'] = bool(flags & SEEK_ACTIVE)
        data['error'] = bool(flags & SEEK_ERROR)

    elif cmd[0] == LINE:
        data['type'] = 'line'
        data['exit-vel']  = decode_float(cmd[1:7])
        data['max-accel'] = decode_float(cmd[7:13])
        data['max-jerk']  = decode_float(cmd[13:19])

        data['target'] = {}
        data['times'] = [0] * 7
        cmd = cmd[19:]

        while len(cmd):
            name = cmd[0]
            value = decode_float(cmd[1:7])
            cmd = cmd[7:]

            if name in 'xyzabcuvw': data['target'][name] = value
            else: data['times'][int(name)] = value

    elif cmd[0] == REPORT:  data['type'] = 'report'
    elif cmd[0] == PAUSE:   data['type'] = 'pause'
    elif cmd[0] == UNPAUSE: data['type'] = 'unpause'
    elif cmd[0] == ESTOP:   data['type'] = 'estop'
    elif cmd[0] == CLEAR:   data['type'] = 'clear'
    elif cmd[0] == FLUSH:   data['type'] = 'flush'
    elif cmd[0] == RESUME:  data['type'] = 'resume'

    print(json.dumps(data))


def decode(cmd):
    for line in cmd.split('\n'):
        decode_command(line.strip())


if __name__ == "__main__":
    import sys

    if 1 < len(sys.argv):
        for arg in sys.argv[1:]:
            decode(arg)

    else:
        for line in sys.stdin:
            decode(line)
