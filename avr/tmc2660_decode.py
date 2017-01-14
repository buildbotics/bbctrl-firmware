#!/usr/bin/env python3

import sys
import csv
import json


rdsel = 1


def twos_comp(val, bits):
    if val & (1 << (bits - 1)): return val - (1 << bits)
    return val


def tmc2660_decode_response(x, rdsel = 1):
    x >>= 4 # Shift right 4 bits
    d = {'_hex': '0x%05x' % x}

    if rdsel == 0:
        d['mstep'] = (x >> 10) & 0x3ff

    elif rdsel == 1:
        d['sg'] = (x >> 10) & 0x3ff

    elif rdsel == 2:
        d['sg'] = x >> 15 & 0x1f
        d['se'] = (x >> 10) & 0x1f

    flags = []
    if x & (1 << 7): flags += ['stand']
    if x & (1 << 6): flags += ['open B']
    if x & (1 << 5): flags += ['open A']
    if x & (1 << 4): flags += ['short B']
    if x & (1 << 3): flags += ['short A']
    if x & (1 << 2): flags += ['temp warn']
    if x & (1 << 1): flags += ['overtemp']
    if x & (1 << 0): flags += ['stall']

    d['flags'] = flags

    return d


def tmc2660_decode_cmd(x, r):
    global rdsel

    addr = x >> 17

    d = {'_hex': '0x%05x' % x}

    if addr == 0 or addr == 1:
        cmd = 'DRVCTRL'
        d['mstep'] = (256, 128, 64, 32, 16, 8, 4, 2, 1)[x & 0xf]
        d['dedge'] = bool(x & (1 << 8))
        d['intpol'] = bool(x & (1 << 9))

    elif addr == 4:
        chm = bool(x & (1 << 14))

        cmd = 'CHOPCONF'
        d['blank'] = (16, 24, 36, 54)[(x >> 15) & 3]
        d['chm'] = 'standard' if chm else 'constant'
        d['random toff'] = bool(x & (1 << 13))

        if chm: d['fast decay mode'] = ('current', 'timer')[x >> 12]
        else: d['hdec'] = (16, 32, 48, 64)[(x >> 11) & 3]

        if chm: d['SWO'] = ((x >> 7) & 0xf) - 3
        else: d['HEND'] = ((x >> 7) & 0xf) - 3

        if chm: d['fast decay'] = (((x >> 4) & 7) + ((x >> 7) & 8)) * 32
        else: d['HSTART'] = ((x >> 4) & 7) + 1

        # TODO this isn't quite right
        toff = x & 0xf
        if toff == 0: d['TOFF'] = 'disabled'
        else: d['TOFF'] = 12 + (32 * toff)

    elif addr == 5:
        cmd = 'SMARTEN'
        d['SEIMIN'] = '1/4 CS' if x & (1 << 15) else '1/2 CS'
        d['SEDN'] = (32, 8, 2, 1)[(x >> 13) & 3]
        d['SEMAX'] = (x >> 8) & 0xf
        d['SEUP'] = (1, 2, 4, 8)[(x >> 5) & 3]
        semin = x & 0xf
        d['SEMIN'] = semin if semin else 'disabled'

    elif addr == 6:
        cmd = 'SGCSCONF'
        d['SFILT'] = bool(x & (1 << 16))
        d['SGT'] = twos_comp((x >> 8) & 0x7f, 7)
        d['CS'] = ((x & 0x1f) + 1) / 32.0

    elif addr == 7:
        cmd = 'DRVCONF'
        d['TST'] = bool(x & (1 << 16))
        d['SLPH'] = ('min', 'min temp', 'med temp', 'max')[(x >> 14) & 3]
        d['SLPL'] = ('min', 'min', 'med', 'max')[(x >> 12) & 3]
        d['DISS2G'] = bool(x & (1 << 10))
        d['TS2G'] = (3.2, 1.6, 1.2, 0.8)[(x >> 8) & 3]
        d['SDOFF'] = 'SPI' if x & (1 << 7) else 'step'
        d['VSENSE'] = '165mV' if x & (1 << 6) else '305mV'
        rdsel = (x >> 4) & 3
        d['RDSEL'] = ('mstep', 'SG', 'SG & CS', 'Invalid')[rdsel]

    else: cmd = 'INVALID'

    return {cmd: d, 'response': tmc2660_decode_response(r, rdsel)}


def tmc2660_decode(path):
    with open(path, newline = '') as f:
        reader = csv.reader(f)

        first = True
        for time, packet, mosi, miso in reader:
            if first:
                first = False
                continue

            cmd = tmc2660_decode_cmd(int(mosi, 0), int(miso, 0))
            cmd['id'] = int(packet)
            cmd['ts'] = float(time)

            print(json.dumps(cmd, sort_keys = True))


if __name__ == "__main__":
    for path in sys.argv[1:]:
        tmc2660_decode(path)
