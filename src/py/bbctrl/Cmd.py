import struct
import base64
import logging

log = logging.getLogger('Cmd')

# TODO, sync this up with AVR code
SET      = '$'
SET_SYNC = '#'
SEEK     = 's'
LINE     = 'l'
REPORT   = 'r'
PAUSE    = 'P'
UNPAUSE  = 'U'
ESTOP    = 'E'
CLEAR    = 'C'
FLUSH    = 'F'
STEP     = 'S'
RESUME   = 'c'

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

    return data


def seek(switch, open, error):
    flags = (SEEK_OPEN if open else 0) | (SEEK_ERROR if error else 0)
    return '%c%x%x' % (SEEK, switch, flags)


def line_number(line): return '#ln=%d' % line


def line(id, target, exitVel, maxAccel, maxJerk, times):
    cmd = '#id=%u\n%c' % (id, LINE)

    cmd += encode_float(exitVel)
    cmd += encode_float(maxAccel)
    cmd += encode_float(maxJerk)
    cmd += encode_axes(target)

    # S-Curve time parameters
    for i in range(7):
        if times[i]:
            cmd += str(i) + encode_float(times[i] / 60000) # to mins

    return cmd


def tool(tool): return '#t=%d' % tool
def speed(speed): return '#s=:' + encode_float(speed)
def dwell(seconds): return 'd' + encode_float(seconds)
def pause(optional = False): 'P' + ('1' if optional else '0')
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
