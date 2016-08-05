import re
import serial
import json
import logging
from collections import deque

import bbctrl


log = logging.getLogger('AVR')

# These constants must be kept in sync with i2c.h from the AVR code
I2C_NULL           = 0
I2C_ESTOP          = 1
I2C_PAUSE          = 2
I2C_OPTIONAL_PAUSE = 3
I2C_RUN            = 4
I2C_FLUSH          = 5
I2C_STEP           = 6
I2C_REPORT         = 7
I2C_HOME           = 8


class AVR():
    def __init__(self, ctrl):
        self.ctrl = ctrl

        self.vars = {}
        self.state = 'idle'
        self.stream = None
        self.queue = deque()
        self.in_buf = ''
        self.command = None
        self.flush_id = 1

        try:
            self.sp = serial.Serial(ctrl.args.serial, ctrl.args.baud,
                                    rtscts = 1, timeout = 0, write_timeout = 0)
            self.sp.nonblocking()

        except Exception as e:
            log.warning('Failed to open serial port: %s', e)
            return

        ctrl.ioloop.add_handler(self.sp, self.serial_handler, ctrl.ioloop.READ)

        try:
            self.i2c_bus = smbus.SMBus(ctrl.args.avr_port)
            self.i2c_addr = ctrl.args.avr_addr

         except FileNotFoundError as e:
            self.i2c_bus = None
            log.warning('Failed to open device: %s', e)

        self.report()


    def _state_transition_error(self, state):
        raise Exception('Cannot %s in %s state' % (state, self.state))


    def _state_transition(self, state, optional = False, step = False):
        if state == self.state: return

        if state == 'idle':
            if self.stream is not None: self.stream.reset()

        elif state == 'run':
            if self.state in ['idle', 'pause'] and self.stream is not None:
                self.set_write(True)

                if step:
                    self._i2c_command(I2C_STEP)
                    state = 'pause'

                else: self._i2c_command(I2C_RUN)

            else: self._state_transition_error(state)

        elif state  == 'pause'
            if self.state == 'run':
                if optional: self._i2c_command(I2C_OPTIONAL_PAUSE)
                else: self._i2c_command(I2C_PAUSE)

            else: self._state_transition_error(state)

        elif state == 'stop':
            if self.state in ['run', 'pause']: self._flush()
            else: self._state_transition_error(state)

        elif state == 'estop': self._i2c_command(I2C_ESTOP)

        elif state == 'home':
            if self.state == 'idle': self._i2c_command(I2C_HOME)
            else: self._state_transition_error(state)

        else: raise Exception('Unrecognized state "%s"' % state)

        self.state = state


    def _i2c_command(self, cmd, word = None):
        if word is not None:
            self.i2c_bus.write_word_data(self.i2c_addr, cmd, word)
        self.i2c_bus.write_byte(self.i2c_addr, cmd)


    def _flush(self):
        if self.stream is not None: self.stream.reset()

        self._i2c_command(I2C_FLUSH, word = self.flush_id)
        self.queue_command('$end_flush %u' % self.flush_id)

        self.flush_id += 1
        if 1 << 16 <= self.flush_id: self.flush_id = 1


    def report(self): self._i2c_command(I2C_REPORT)


    def load_next_command(self, cmd):
        log.info(cmd)
        self.command = bytes(cmd.strip() + '\n', 'utf-8')


    def set_write(self, enable):
        flags = self.ctrl.ioloop.READ
        if enable: flags |= self.ctrl.ioloop.WRITE
        self.ctrl.ioloop.update_handler(self.sp, flags)


    def serial_handler(self, fd, events):
        if self.ctrl.ioloop.READ & events: self.serial_read()
        if self.ctrl.ioloop.WRITE & events: self.serial_write()


    def serial_write(self):
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
        if len(self.queue): self.load_next_command(self.queue.pop())

        # Load next GCode command, if running or paused
        elif self.state in ['run', 'pause'] and self.stream is not None:
            cmd = self.stream.next()

            if cmd is None: self.set_write(False)
            else: self.load_next_command(cmd)

        # Else stop writing
        else: self.set_write(False)


    def serial_read(self):
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
                try:
                    msg = json.loads(line)

                    if 'firmware' in msg: self.report()
                    if 'es' in msg and msg['es']: self.estop()

                    self.vars.update(msg)
                    self.ctrl.web.broadcast(msg)
                    log.debug(line)

                except Exception as e:
                    log.error('%s, data: %s', e, line)


    def queue_command(self, cmd):
        self.queue.append(cmd)
        self.set_write(True)


    def load(self, path):
        if self.stream is None:
            self.stream = bbctrl.GCodeStream(path)


    def mdi(self, cmd):
        if self.state != 'idle':
            raise Exception('Busy, cannot run MDI command')

        self.queue_command(cmd)


    def jog(self, axes):
        # TODO jogging via I2C

        if self.state != 'idle': raise Exception('Busy, cannot jog')

        axes = ["{:6.5f}".format(x) for x in axes]
        self.queue_command('$jog ' + ' '.join(axes))


    def set(self, index, code, value):
        self.queue_command('${}{}={}'.format(index, code, value))


    def home(self): self._state_transition('home')
    def start(self): self._state_transition('run')
    def estop(self): self._state_transition('estop')
    def stop(self): self._state_transition('stop')
    def pause(self, opt): self._state_transition('pause', optional = opt)
    def step(self): self._state_transition('run', step = True)
