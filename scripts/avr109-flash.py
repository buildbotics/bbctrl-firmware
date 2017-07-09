#!/usr/bin/env python3

import sys
import time
import serial
import binascii
from subprocess import call


dev = '/dev/ttyAMA0'
baud = 921600
boot_id = 'bbctrl '
verbose = False


def crc16(data):
    crc = 0xffff

    for x in data:
        crc = crc ^ int(x)
        for bit in range(8):
            if crc & 1: crc = (crc >> 1) ^ 0xa001
            else: crc = crc >> 1

    return crc


def avr_crc32(data, length):
    mem = [0xff] * length

    for addr, chunk in data:
        for x in chunk:
            mem[addr] = x
            addr += 1

    return binascii.crc32(bytes(mem))


def read_intel_hex(f):
    base = 0
    pos = 0
    start = 0
    chunk = None

    for line in f:
        line = line.strip()
        if not line or line[0] != ':': continue

        count = int(line[1:3], 16)
        addr = int(line[3:7], 16)
        type = int(line[7:9], 16)
        data = line[9:-2]
        checksum = int(line[-2:], 16)

        if type == 0:
            addr += base
            data = binascii.unhexlify(bytes(data, 'utf8'))

            if chunk is None or pos != addr:
                if chunk is not None: yield (start, chunk)
                start = addr
                chunk = data

            else: chunk += data

            pos = addr + len(data)

        if type == 2: base = int(data, 16) * 16

    if chunk is not None: yield (start, chunk)


def send(data):
    if verbose: print('Sending:', data)
    sp.write(bytes(data, 'utf8'))


def send_int(x, size):
    if verbose: print('Sending: %d', x)
    sp.write((x).to_bytes(size, byteorder = 'big'))


def recv(count):
    data = sp.read(count).decode('utf8')
    if count and verbose: print('Received:', data)
    return data


def recv_int(size):
    x = int.from_bytes(sp.read(size), byteorder = 'big')
    if verbose: print('Received:', x)
    return x


# Read firmware hex file
data = list(read_intel_hex(open(sys.argv[1], 'r')))

# Open serial  port
sp = serial.Serial(dev, baud, timeout = 10)

# Reset AVR
call(['gpio', '-g', 'write', '27', '0'])
call(['gpio', '-g', 'write', '27', '1'])
time.sleep(0.1)

# Sync
for i in range(10): send('\x1b')

# Flush serial
try:
    recv(sp.in_waiting)
except: pass

# Get bootloader ID
send('S')
if boot_id != recv(len(boot_id)):
    raise Exception('Failed to communicate with bootloader')

# Get version
send('V')
major = int(recv(1))
minor = int(recv(1))
print('Bootloader version: %d.%d' % (major, minor))

# If bootloader is new enough compare checksums
if 0 < major or 1 < minor:
    # Get flash length
    send('n')
    flash_len = recv_int(3)

    # Get current flash CRC
    send('X')
    new_crc = avr_crc32(data, flash_len)
    old_crc = recv_int(4)
    if old_crc == new_crc:
        print('Flash already up to date')
        sys.exit(0)

    print('CRC: old=0x%08x new=0x%08x' % (old_crc, new_crc))

    # Erase
    send('e')
    if recv(1) != '\r': raise Exception('Flash erase failed')

# Get page size
send('b')
if recv(1) != 'Y': raise Exception('Cannot get page size')
page_size = recv_int(2)
print('Page size:', page_size)


# Program
print('Programming', end = '')
count = 0
retry = 0
for addr, chunk in data:
    # Set address
    send('H')
    send_int(addr, 3)
    if recv(1) != '\r': raise Exception('Failed to set address')

    while len(chunk):
        sys.stdout.flush()
        page = chunk[0:512]

        # Block command
        send('B')

        # Send block size
        send_int(len(page), 2)

        # Send memory type
        send('F')

        # Send block
        sp.write(page)

        if recv(1) != '\r': raise Exception('Failed to write block')

        # Get and check block CRC
        send('i')
        crc = recv_int(2)
        expect = crc16(page)
        if crc != expect:
            retry += 1
            if retry == 5:
                raise Exception('CRC mismatch %d != %d' % (crc, expect))

            print('x', end = '')
            continue

        count += len(page)
        chunk = chunk[512:]
        retry = 0
        print('.', end = '')


print('done')
print('Wrote %d bytes' % count)

# Exit bootloader
send('E')
