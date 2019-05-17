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

import os
import sys
import traceback
import signal

import bbctrl
import bbctrl.Cmd as Cmd


class AVREmu(object):
    def __init__(self, ctrl):
        self.ctrl = ctrl
        self.log = ctrl.log.get('AVREmu')

        self.avrOut   = None
        self.avrIn    = None
        self.i2cOut   = None
        self.read_cb  = None
        self.write_cb = None
        self.pid      = None


    def close(self):
        # Close pipes
        def _close(fd, withHandle):
            if fd is None: return
            try:
                if withHandle: self.ctrl.ioloop.remove_handler(fd)
            except: pass
            try:
                os.close(fd)
            except: pass

        _close(self.avrOut, True)
        _close(self.avrIn,  True)
        _close(self.i2cOut, False)

        self.avrOut, self.avrIn, self.i2cOut = None, None, None

        # Kill process and wait for it
        if self.pid is not None:
            os.kill(self.pid, signal.SIGKILL)
            os.waitpid(self.pid, 0)
            self.pid = None


    def _start(self):
        try:
            self.close()

            # Create pipes
            stdinFDs  = os.pipe()
            stdoutFDs = os.pipe()
            i2cFDs    = os.pipe()

            self.pid = os.fork()

            if not self.pid:
                # Dup child ends
                os.dup2(stdinFDs[0],  0)
                os.dup2(stdoutFDs[1], 1)
                os.dup2(i2cFDs[0],    3)

                # Close orig fds
                os.close(stdinFDs[0])
                os.close(stdoutFDs[1])
                os.close(i2cFDs[0])

                # Close parent ends
                os.close(stdinFDs[1])
                os.close(stdoutFDs[0])
                os.close(i2cFDs[1])

                cmd = ['bbemu']
                if self.ctrl.args.fast_emu: cmd.append('--fast')

                os.execvp(cmd[0], cmd)
                os._exit(1) # In case of failure

            # Parent, close child ends
            os.close(stdinFDs[0])
            os.close(stdoutFDs[1])
            os.close(i2cFDs[0])

            # Non-blocking IO
            os.set_blocking(stdinFDs[1],  False)
            os.set_blocking(stdoutFDs[0], False)
            os.set_blocking(i2cFDs[1],    False)

            self.avrOut = stdinFDs[1]
            self.avrIn  = stdoutFDs[0]
            self.i2cOut = i2cFDs[1]

            ioloop = self.ctrl.ioloop
            ioloop.add_handler(self.avrOut, self._avr_write_handler,
                               ioloop.WRITE | ioloop.ERROR)
            ioloop.add_handler(self.avrIn, self._avr_read_handler,
                               ioloop.READ | ioloop.ERROR)

            self.write_enabled = True

        except Exception:
            self.close()
            self.log.exception('Failed to start bbemu')


    def set_handlers(self, read_cb, write_cb):
        if self.read_cb is not None or self.write_cb is not None:
            raise Exception('AVR handler already set')

        self.read_cb = read_cb
        self.write_cb = write_cb
        self._start()


    def enable_write(self, enable):
        if self.avrOut is None: return

        flags = self.ctrl.ioloop.WRITE if enable else 0
        self.ctrl.ioloop.update_handler(self.avrOut, flags)
        self.write_enabled = enable


    def _avr_write(self, data):
        try:
            length = os.write(self.avrOut, data)
            self.continue_write = length and length == len(data)
            return length

        except BlockingIOError: pass
        except BrokenPipeError: pass

        return 0


    def _avr_write_handler(self, fd, events):
        if self.avrOut is None: return

        if events & self.ctrl.ioloop.ERROR:
            self._start()
            return

        try:
            while True:
                self.continue_write = False
                self.write_cb(self._avr_write)
                if not self.continue_write: break

        except Exception as e:
            self.log.warning('AVR write handler error: %s',
                             traceback.format_exc())


    def _avr_read_handler(self, fd, events):
        if self.avrIn is None: return

        if events & self.ctrl.ioloop.ERROR:
            self._start()
            return

        try:
            data = os.read(self.avrIn, 4096)
            if data is not None: self.read_cb(data)

        except Exception as e:
            self.log.warning('AVR read handler error: %s %s' %
                        (data, traceback.format_exc()))


    def i2c_command(self, cmd, byte = None, word = None, block = None):
        if byte is not None: data = chr(byte)
        elif word is not None: data = word
        elif block is not None: data = block
        else: data = ''

        try:
            if self.i2cOut is not None:
                os.write(self.i2cOut, bytes(cmd + data + '\n', 'utf-8'))

        except BrokenPipeError: pass
