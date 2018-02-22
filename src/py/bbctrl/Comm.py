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

import serial
import logging
import json
import time
import traceback
from collections import deque

import bbctrl
import bbctrl.Cmd as Cmd

log = logging.getLogger('Comm')


class Comm():
    def __init__(self, ctrl, next_cb, connect_cb):
        self.ctrl = ctrl
        self.next_cb = next_cb
        self.connect_cb = connect_cb

        self.queue = deque()
        self.in_buf = ''
        self.command = None

        try:
            self.sp = serial.Serial(ctrl.args.serial, ctrl.args.baud,
                                    rtscts = 1, timeout = 0, write_timeout = 0)
            self.sp.nonblocking()

        except Exception as e:
            self.sp = None
            log.warning('Failed to open serial port: %s', e)

        if self.sp is not None:
            ctrl.ioloop.add_handler(self.sp, self._serial_handler,
                                    ctrl.ioloop.READ)

        self.i2c_addr = ctrl.args.avr_addr


    def is_active(self):
        return len(self.queue) or self.command is not None


    def i2c_command(self, cmd, byte = None, word = None):
        log.info('I2C: ' + cmd)
        retry = 5
        cmd = ord(cmd[0])

        while True:
            try:
                self.ctrl.i2c.write(self.i2c_addr, cmd, byte, word)
                break

            except Exception as e:
                retry -= 1

                if retry:
                    log.warning('AVR I2C failed, retrying: %s' % e)
                    time.sleep(0.1)
                    continue

                else:
                    log.error('AVR I2C failed: %s' % e)
                    raise


    def set_write(self, enable):
        if self.sp is None: return

        flags = self.ctrl.ioloop.READ
        if enable: flags |= self.ctrl.ioloop.WRITE
        self.ctrl.ioloop.update_handler(self.sp, flags)


    def _load_next_command(self, cmd):
        log.info('< ' + json.dumps(cmd).strip('"'))
        self.command = bytes(cmd.strip() + '\n', 'utf-8')


    def queue_command(self, cmd):
        self.queue.append(cmd)
        self.set_write(True)


    def _serial_write(self):
        # Finish writing current command
        if self.command is not None:
            try:
                count = self.sp.write(self.command)

            except Exception as e:
                self.set_write(False)
                raise e

            self.command = self.command[count:]
            if len(self.command): return # There's more
            self.command = None

        # Load next command from queue
        if len(self.queue): self._load_next_command(self.queue.popleft())

        # Load next command from callback
        else:
            cmd = self.next_cb()

            if cmd is None: self.set_write(False) # Stop writing
            else: self._load_next_command(cmd)


    def _update_vars(self, msg):
        try:
            self.ctrl.state.machine_cmds_and_vars(msg)
            self.queue_command(Cmd.DUMP) # Refresh all vars

        except Exception as e:
            log.warning('AVR reload failed: %s', traceback.format_exc())
            self.ctrl.ioloop.call_later(1, self.connect)


    def _log_msg(self, msg):
        level = msg.get('level', 'info')
        if 'where' in msg: extra = {'where': msg['where']}
        else: extra = None
        msg = msg['msg']

        if   level == 'info':    log.info(msg,    extra = extra)
        elif level == 'debug':   log.debug(msg,   extra = extra)
        elif level == 'warning': log.warning(msg, extra = extra)
        elif level == 'error':   log.error(msg,   extra = extra)


    def _serial_read(self):
        try:
            data = self.sp.read(self.sp.in_waiting)
            self.in_buf += data.decode('utf-8')

        except Exception as e:
            log.warning('%s: %s', e, data)

        # Parse incoming serial data into lines
        while True:
            i = self.in_buf.find('\n')
            if i == -1: break
            line = self.in_buf[0:i].strip()
            self.in_buf = self.in_buf[i + 1:]

            if line:
                log.info('> ' + line)

                try:
                    msg = json.loads(line)

                except Exception as e:
                    log.warning('%s, data: %s', e, line)
                    continue

                if 'variables' in msg:
                    self._update_vars(msg)

                elif 'msg' in msg:
                    self._log_msg(msg)

                elif 'firmware' in msg:
                    log.warning('firmware rebooted')
                    self.connect()

                else: self.ctrl.state.update(msg)


    def _serial_handler(self, fd, events):
        try:
            if self.ctrl.ioloop.READ & events: self._serial_read()
            if self.ctrl.ioloop.WRITE & events: self._serial_write()
        except Exception as e:
            log.warning('Serial handler error: %s', traceback.format_exc())


    def connect(self):
        try:
            # Call connect callback
            self.connect_cb()

            # Resume once current queue of GCode commands has flushed
            self.queue_command(Cmd.RESUME)
            self.queue_command(Cmd.HELP) # Load AVR commands and variables

        except Exception as e:
            log.warning('Connect failed: %s', e)
            self.ctrl.ioloop.call_later(1, self.connect)
