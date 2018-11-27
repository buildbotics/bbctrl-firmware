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


class Comm(object):
    def __init__(self, ctrl):
        self.ctrl = ctrl
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


    def comm_next(self): raise Exception('Not implemented')
    def comm_error(self): raise Exception('Not implemented')


    def is_active(self):
        return len(self.queue) or self.command is not None


    def i2c_command(self, cmd, byte = None, word = None, block = None):
        log.info('I2C: %s b=%s w=%s d=%s' % (cmd, byte, word, block))
        retry = 5
        cmd = ord(cmd[0])

        while True:
            try:
                self.ctrl.i2c.write(self.i2c_addr, cmd, byte, word, block)
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


    def _set_write(self, enable):
        if self.sp is None: return

        flags = self.ctrl.ioloop.READ
        if enable: flags |= self.ctrl.ioloop.WRITE
        self.ctrl.ioloop.update_handler(self.sp, flags)


    def flush(self): self._set_write(True)


    def _load_next_command(self, cmd):
        log.info('< ' + json.dumps(cmd).strip('"'))
        self.command = bytes(cmd.strip() + '\n', 'utf-8')


    def resume(self): self.queue_command(Cmd.RESUME)


    def queue_command(self, cmd):
        self.queue.append(cmd)
        self.flush()


    def _serial_write(self):
        # Finish writing current command
        if self.command is not None:
            try:
                count = self.sp.write(self.command)

            except Exception as e:
                self.command = None
                raise e

            self.command = self.command[count:]
            if len(self.command): return # There's more
            self.command = None

        # Load next command from queue
        if len(self.queue): self._load_next_command(self.queue.popleft())

        # Load next command from callback
        else:
            cmd = self.comm_next() # pylint: disable=assignment-from-no-return

            if cmd is None: self._set_write(False) # Stop writing
            else: self._load_next_command(cmd)


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

        if level == 'error': self.comm_error()


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

                if 'variables' in msg: self._update_vars(msg)
                elif 'msg' in msg: self._log_msg(msg)

                elif 'firmware' in msg:
                    log.info('AVR firmware rebooted')
                    self.connect()

                else:
                    self.ctrl.state.update(msg)
                    if 'xx' in msg:           # State change
                        self.ctrl.ready()     # We've received data from AVR
                        self.flush()          # May have more data to send now


    def _serial_handler(self, fd, events):
        try:
            if self.ctrl.ioloop.READ & events: self._serial_read()
            if self.ctrl.ioloop.WRITE & events: self._serial_write()
        except Exception as e:
            log.warning('Serial handler error: %s', traceback.format_exc())


    def estop(self):
        if self.ctrl.state.get('xx', '') != 'ESTOPPED':
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
            log.warning('Connect failed: %s', e)
            self.ctrl.ioloop.call_later(1, self.connect)
