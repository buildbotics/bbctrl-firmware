import re
import serial
import json
import logging
from collections import deque

try:
    import smbus
except:
    import smbus2 as smbus

import bbctrl


log = logging.getLogger('AVR')

# These constants must be kept in sync with i2c.h from the AVR code
I2C_NULL           = 0
I2C_ESTOP          = 1
I2C_CLEAR          = 2
I2C_PAUSE          = 3
I2C_OPTIONAL_PAUSE = 4
I2C_RUN            = 5
I2C_STEP           = 6
I2C_FLUSH          = 7
I2C_REPORT         = 8
I2C_HOME           = 9
I2C_REBOOT         = 10


class AVR():
    def __init__(self, ctrl):
        self.ctrl = ctrl

        self.vars = {}
        self.stream = None
        self.queue = deque()
        self.in_buf = ''
        self.command = None

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


    def _start_sending_gcode(self, path):
        if self.stream is not None:
            raise Exception('Busy, cannot start new GCode file')

        self.stream = bbctrl.GCodeStream(path)
        self.set_write(True)


    def _stop_sending_gcode(self):
        if self.stream is not None:
            self.stream.reset()
            self.stream = None


    def _i2c_command(self, cmd, word = None):
        if not hasattr(self, 'i2c_bus'): return

        log.info('I2C: %d' % cmd)

        if word is not None:
            self.i2c_bus.write_word_data(self.i2c_addr, cmd, word)
        self.i2c_bus.write_byte(self.i2c_addr, cmd)


    def report(self): self._i2c_command(I2C_REPORT)


    def load_next_command(self, cmd):
        log.info('Serial: ' + cmd)
        self.command = bytes(cmd.strip() + '\n', 'utf-8')


    def set_write(self, enable):
        if not hasattr(self, 'sp'): return

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
        elif self.stream is not None:
            cmd = self.stream.next()

            if cmd is None:
                self.set_write(False)
                self.stream = None

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

                except Exception as e:
                    log.error('%s, data: %s', e, line)

                if 'firmware' in msg:
                    log.error('AVR rebooted')
                    self._stop_sending_gcode()
                    self.report()

                if 'x' in msg and msg['x'] == 'estopped':
                    self._stop_sending_gcode()

                self.vars.update(msg)
                self.ctrl.lcd.update(msg)
                self.ctrl.web.broadcast(msg)

                log.debug(line)



    def queue_command(self, cmd):
        self.queue.append(cmd)
        self.set_write(True)


    def mdi(self, cmd):
        if self.stream is not None:
            raise Exception('Busy, cannot queue MDI command')

        self.queue_command(cmd)


    def jog(self, axes):
        if self.stream is not None: raise Exception('Busy, cannot jog')

        # TODO jogging via I2C

        axes = ["{:6.5f}".format(x) for x in axes]
        self.queue_command('$jog ' + ' '.join(axes))


    def set(self, index, code, value):
        self.queue_command('${}{}={}'.format(index, code, value))


    def home(self): self._i2c_command(I2C_HOME)
    def estop(self): self._i2c_command(I2C_ESTOP)
    def clear(self): self._i2c_command(I2C_CLEAR)


    def start(self, path):
        if self.stream is not None: raise Exception('Busy, cannot start file')

        if path:
            self._start_sending_gcode(path)
            self._i2c_command(I2C_RUN)


    def stop(self):
        self._i2c_command(I2C_FLUSH)
        self._stop_sending_gcode()
        # Resume processing once current queue of GCode commands has flushed
        self.queue_command('$resume')

    def pause(self): self._i2c_command(I2C_PAUSE)
    def unpause(self): self._i2c_command(I2C_RUN)
    def optional_pause(self): self._i2c_command(I2C_OPTIONAL_PAUSE)
    def step(self): self._i2c_command(I2C_STEP)
