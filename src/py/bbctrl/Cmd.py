import struct
import base64
import logging

log = logging.getLogger('Cmd')

# TODO, sync this up with AVR code
REPORT = 'r'
PAUSE  = 'P'
ESTOP  = 'E'
CLEAR  = 'C'
FLUSH  = 'F'
STEP   = 'S'
RUN    = 'p'


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


def line_number(line): return '#ln=%d' % line


def line(target, exitVel, maxJerk, times):
    data = 'l'

    data += encode_float(exitVel)
    data += encode_float(maxJerk)
    data += encode_axes(target)

    # S-Curve time parameters
    for i in range(7):
        if times[i]:
            data += str(i) + encode_float(times[i] / 60000) # to mins

    return data


def tool(tool): return '#t=%d' % tool
def speed(speed): return '#s=:' + encode_float(speed)
def dwell(seconds): return 'd' + encode_float(seconds)
def pause(optional = False): 'P' + ('1' if optional else '0')
def jog(axes): return 'j' + encode_axes(axes)
