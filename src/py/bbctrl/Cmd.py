#!/usr/bin/env python3

################################################################################
#                                                                              #
#                 This file is part of the Buildbotics firmware.               #
#                                                                              #
#        Copyright (c) 2015 - 2021, Buildbotics LLC, All rights reserved.      #
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

import struct
import base64
import json

# Keep this in sync with AVR code command.def
SET          = '$'
SET_SYNC     = '#'
MODBUS_READ  = 'm'
MODBUS_WRITE = 'M'
SEEK         = 's'
SET_AXIS     = 'a'
LINE         = 'l'
SYNC_SPEED   = '%'
SPEED        = 'p'
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
SHUTDOWN     = 'X'
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


def line(target, exitVel, maxAccel, maxJerk, times, speeds):
    cmd = LINE

    cmd += encode_float(exitVel)
    cmd += encode_float(maxAccel)
    cmd += encode_float(maxJerk)
    cmd += encode_axes(target)

    # S-Curve time parameters
    for i in range(7):
        if times[i]:
            cmd += str(i) + encode_float(times[i] / 60000) # to mins

    # Speeds
    for dist, speed in speeds:
        cmd += '\n' + sync_speed(dist, speed)

    return cmd


def speed(value): return SPEED + encode_float(value)


def sync_speed(dist, speed):
    return SYNC_SPEED + encode_float(dist) + encode_float(speed)


def _get_input_type_index(port):
    if port == 'digital-in-0': return 'd', 0
    if port == 'digital-in-1': return 'd', 1
    if port == 'digital-in-2': return 'd', 2
    if port == 'digital-in-3': return 'd', 3
    if port == 'analog-in-0':  return 'a', 0
    if port == 'analog-in-1':  return 'a', 1
    if port == 'analog-in-2':  return 'a', 2
    if port == 'analog-in-3':  return 'a', 3

    raise Exception('Unsupported input "%s"' % port)


def _get_input_mode(mode):
    if mode == 'immediate': return 0
    if mode == 'rise':      return 1
    if mode == 'fall':      return 2
    if mode == 'high':      return 3
    if mode == 'low':       return 4

    raise Exception('Unsupported input mode "%s"' % mode)


def input(port, mode, timeout):
    type, index = _get_input_type_index(port)
    mode = _get_input_mode(mode)

    return '%s%s%d%d%s' % (INPUT, type, index, mode, encode_float(timeout))


def _get_output_id(port):
    if port == 'digital-out-0': return '0'
    if port == 'digital-out-1': return '1'
    if port == 'digital-out-2': return '2'
    if port == 'digital-out-3': return '3'
    if port == 'mist':          return 'M'
    if port == 'flood':         return 'F'

    raise Exception('Unsupported output "%s"' % port)


def output(port, value):
    return '#%soa=%d' % (_get_output_id(port), 1 if value else 0)


def dwell(seconds): return DWELL + encode_float(seconds)


def _get_pause_type(type):
    if type == 'program':       return 1
    if type == 'optional':      return 2
    if type == 'pallet-change': return 1
    raise Exception('Unknown pause type "%s"' % type)


def pause(type): return '%s%d' % (PAUSE, _get_pause_type(type))
def jog(axes): return JOG + encode_axes(axes)


def seek(switch, active, error):
    cmd = SEEK + '%x' % switch

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

    elif cmd[0] == SYNC_SPEED:
        data['type'] = 'speed'
        data['offset'] = decode_float(cmd[1:7])
        data['speed']  = decode_float(cmd[7:13])

    elif cmd[0] == REPORT:   data['type'] = 'report'
    elif cmd[0] == PAUSE:    data['type'] = 'pause'
    elif cmd[0] == UNPAUSE:  data['type'] = 'unpause'
    elif cmd[0] == ESTOP:    data['type'] = 'estop'
    elif cmd[0] == SHUTDOWN: data['type'] = 'shutdown'
    elif cmd[0] == CLEAR:    data['type'] = 'clear'
    elif cmd[0] == FLUSH:    data['type'] = 'flush'
    elif cmd[0] == RESUME:   data['type'] = 'resume'

    return data


def decode(cmd):
    for line in cmd.split('\n'):
        yield decode_command(line.strip())


def decode_and_print(cmd):
    for data in decode(cmd):
        print(json.dumps(data))


if __name__ == "__main__":
    import sys

    if 1 < len(sys.argv):
        for arg in sys.argv[1:]:
            decode_and_print(arg)

    else:
        for line in sys.stdin:
            decode_and_print(str(line).strip())
