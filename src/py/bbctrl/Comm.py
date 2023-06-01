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

import serial
import json
import time
import traceback
from collections import deque
from abc import *

from . import Cmd

__all__ = ['Comm']


# Must be kept in sync with drv8711.h
DRV8711_STATUS_OTS_bm    = 1 << 0
DRV8711_STATUS_AOCP_bm   = 1 << 1
DRV8711_STATUS_BOCP_bm   = 1 << 2
DRV8711_STATUS_APDF_bm   = 1 << 3
DRV8711_STATUS_BPDF_bm   = 1 << 4
DRV8711_STATUS_UVLO_bm   = 1 << 5
DRV8711_STATUS_STD_bm    = 1 << 6
DRV8711_STATUS_STDLAT_bm = 1 << 7
DRV8711_COMM_ERROR_bm    = 1 << 8

# Ignoring stall and stall latch flags for now
DRV8711_MASK = ~(DRV8711_STATUS_STD_bm | DRV8711_STATUS_STDLAT_bm)


def _driver_flags_to_string(flags):
    if DRV8711_STATUS_OTS_bm    & flags: yield 'over temp'
    if DRV8711_STATUS_AOCP_bm   & flags: yield 'over current a'
    if DRV8711_STATUS_BOCP_bm   & flags: yield 'over current b'
    if DRV8711_STATUS_APDF_bm   & flags: yield 'driver fault a'
    if DRV8711_STATUS_BPDF_bm   & flags: yield 'driver fault b'
    if DRV8711_STATUS_UVLO_bm   & flags: yield 'undervoltage'
    if DRV8711_STATUS_STD_bm    & flags: yield 'stall'
    if DRV8711_STATUS_STDLAT_bm & flags: yield 'stall latch'
    if DRV8711_COMM_ERROR_bm    & flags: yield 'comm error'


def driver_flags_to_string(flags):
    return ', '.join(_driver_flags_to_string(flags))


class Comm(ABC):
    def __init__(self, ctrl, avr):
        self.ctrl = ctrl
        self.avr = avr
        self.log = self.ctrl.log.get('Comm')
        self.queue = deque()
        self.in_buf = ''
        self.command = None
        self.last_motor_flags = [0] * 4
        self.estopped = False

        avr.set_handlers(self._read, self._write)
        self._poll_cb(False)

        # Let simulations proceed after timeout
        ctrl.ioloop.call_later(10, self.ctrl.ready)

    @abstractmethod
    def comm_next(self): pass
    @abstractmethod
    def comm_error(self): pass


    def is_active(self): return len(self.queue) or self.command is not None


    def i2c_command(self, cmd, byte = None, word = None, block = None):
        self.log.info('I2C: %s b=%s w=%s d=%s' % (cmd, byte, word, block))
        self.avr.i2c_command(cmd, byte, word, block)


    def i2c_block(self, block): self.i2c_command(block[0], block = block[1:])
    def i2c_set(self, name, value): self.i2c_block(Cmd.set(name, value))


    def modbus_read(self, addr): self.i2c_block(Cmd.modbus_read(addr))


    def modbus_write(self, addr, value):
        self.i2c_block(Cmd.modbus_write(addr, value))


    def flush(self): self.avr.enable_write(True)


    def _prep_command(self, cmd):
        self.log.info('< ' + json.dumps(cmd).strip('"'))
        return bytes(cmd.strip() + '\n', 'utf-8')


    def resume(self): self.queue_command(Cmd.RESUME)


    def queue_command(self, cmd):
        self.queue.append(cmd)
        self.flush()


    def _poll_cb(self, now = True):
        # Checks periodically for new commands from planner via comm_next()
        if now: self.flush()
        self.ctrl.ioloop.call_later(1, self._poll_cb)


    def _write(self, write_cb):
        # Finish writing current command
        if self.command is not None:
            try:
                count = write_cb(self.command)

            except Exception as e:
                self.command = None
                raise e

            self.command = self.command[count:]
            if len(self.command): return # There's more
            self.command = None

        # Load next command from queue
        if len(self.queue):
            self.command = self._prep_command(self.queue.popleft())

        # Load next command from callback
        else:
            cmd = self.comm_next() # pylint: disable=assignment-from-no-return

            if cmd is None: self.avr.enable_write(False) # Stop writing
            else: self.command = self._prep_command(cmd)


    def _update_vars(self, msg):
        try:
            self.ctrl.state.set_machine_vars(msg['variables'])
            self.ctrl.configure()
            self.queue_command(Cmd.DUMP) # Refresh all vars

            # Set axis positions
            for axis in 'xyzabc':
                position = self.ctrl.state.get(axis + 'p', 0)
                self.queue_command(Cmd.set_axis(axis, position))

        except Exception as e:
            self.log.warning('AVR reload failed: %s', traceback.format_exc())
            self.ctrl.ioloop.call_later(1, self.connect)


    def _log_msg(self, msg):
        level = msg.get('level', 'info')
        where = msg.get('where')
        msg = msg['msg']

        if   level == 'info':    self.log.info(msg,    where = where)
        elif level == 'debug':   self.log.debug(msg,   where = where)
        elif level == 'warning': self.log.warning(msg, where = where)
        elif level == 'error':   self.log.error(msg,   where = where)

        if level == 'error': self.comm_error()

        # Treat machine alarmed warning as an error
        if level == 'warning' and 'code' in msg and msg['code'] == 11:
            self.comm_error()


    def _log_motor_flags(self, update):
        for motor in range(3):
            var = '%ddf' % motor

            if var in update:
                flags = update[var] & DRV8711_MASK

                if self.last_motor_flags[motor] == flags: continue
                self.last_motor_flags[motor] = flags

                flags = driver_flags_to_string(flags)
                self.log.info('Motor %d flags: %s' % (motor, flags))


    def _update_state(self, update):
        self.ctrl.state.update(update)

        if 'xx' in update:        # State change
            self.ctrl.ready()     # We've received data from AVR
            self.flush()          # May have more data to send now

        self._log_motor_flags(update)


    def _read(self, data):
        self.in_buf += data.decode('utf-8')

        # Parse incoming serial data into lines
        while True:
            i = self.in_buf.find('\n')
            if i == -1: break
            line = self.in_buf[0:i].strip()
            self.in_buf = self.in_buf[i + 1:]

            if line:
                try:
                    msg = json.loads(line)

                except Exception as e:
                    self.log.warning('%s, data: %s', e, line)
                    continue

                if self.estopped:
                    if not 'firmware' in msg: continue
                    self.estopped = False

                self.log.info('> ' + line)

                if 'variables' in msg: self._update_vars(msg)
                elif 'msg' in msg: self._log_msg(msg)

                elif 'firmware' in msg:
                    self.log.info('AVR firmware rebooted')
                    self.connect()

                else: self._update_state(msg)


    def enter_estop(self): self.estopped = True


    def estop(self):
        if self.ctrl.state.get('xx', '') != 'ESTOPPED':
            self.command = None
            self.queue.clear()
            self.avr.flush_output()
            self.i2c_command(Cmd.ESTOP)


    def clear(self):
        if self.ctrl.state.get('xx', '') == 'ESTOPPED':
            self.i2c_command(Cmd.CLEAR)


    def pause(self):
        self.i2c_command(Cmd.PAUSE, byte = ord('0')) # User pause


    def reboot(self): self.queue_command(Cmd.REBOOT)


    def connect(self):
        try:
            # Resume once current queue of GCode commands has flushed
            self.queue_command(Cmd.RESUME)
            self.queue_command(Cmd.HELP) # Load AVR commands and variables

        except Exception as e:
            self.log.warning('Connect failed: %s', e)
            self.ctrl.ioloop.call_later(1, self.connect)
