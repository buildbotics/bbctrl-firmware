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
import json
import time
import traceback
from collections import deque

import bbctrl
import bbctrl.Cmd as Cmd


class Comm(object):
    def __init__(self, ctrl, avr):
        self.ctrl = ctrl
        self.avr = avr
        self.log = self.ctrl.log.get('Comm')
        self.queue = deque()
        self.in_buf = ''
        self.command = None

        avr.set_handlers(self._read, self._write)


    def comm_next(self): raise Exception('Not implemented')
    def comm_error(self): raise Exception('Not implemented')


    def is_active(self): return len(self.queue) or self.command is not None


    def i2c_command(self, cmd, byte = None, word = None, block = None):
        self.log.info('I2C: %s b=%s w=%s d=%s' % (cmd, byte, word, block))
        self.avr.i2c_command(cmd, byte, word, block)


    def flush(self): self.avr.enable_write(True)


    def _load_next_command(self, cmd):
        self.log.info('< ' + json.dumps(cmd).strip('"'))
        self.command = bytes(cmd.strip() + '\n', 'utf-8')


    def resume(self): self.queue_command(Cmd.RESUME)


    def queue_command(self, cmd):
        self.queue.append(cmd)
        self.flush()


    def _write(self, write):
        # Finish writing current command
        if self.command is not None:
            try:
                count = write(self.command)

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

            if cmd is None: self.avr.enable_write(False) # Stop writing
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


    def _read(self, data):
        self.in_buf += data.decode('utf-8')

        # Parse incoming serial data into lines
        while True:
            i = self.in_buf.find('\n')
            if i == -1: break
            line = self.in_buf[0:i].strip()
            self.in_buf = self.in_buf[i + 1:]

            if line:
                self.log.info('> ' + line)

                try:
                    msg = json.loads(line)

                except Exception as e:
                    self.log.warning('%s, data: %s', e, line)
                    continue

                if 'variables' in msg: self._update_vars(msg)
                elif 'msg' in msg: self._log_msg(msg)

                elif 'firmware' in msg:
                    self.log.info('AVR firmware rebooted')
                    self.connect()

                else:
                    self.ctrl.state.update(msg)
                    if 'xx' in msg:           # State change
                        self.ctrl.ready()     # We've received data from AVR
                        self.flush()          # May have more data to send now


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
            self.log.warning('Connect failed: %s', e)
            self.ctrl.ioloop.call_later(1, self.connect)
