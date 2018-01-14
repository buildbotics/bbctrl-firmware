import re
import serial
import json
import time
import logging
from collections import deque

import bbctrl
import bbctrl.Cmd as Cmd

log = logging.getLogger('AVR')

machine_state_vars = '''
  xp yp zp ap bp cp u s f t fm pa cs ao pc dm ad fo so mc fc
'''.split()

# Axis homing procedure
#   - Set axis unhomed
#   - Find switch
#   - Backoff switch
#   - Find switch more accurately
#   - Backoff to machine zero
#   - Set axis home position
axis_homing_procedure = '''
  G28.2 %(axis)s0 F[#<%(axis)s.sv>]
  G38.6 %(axis)s[#<%(axis)s.hd> * [#<%(axis)s.tm> - #<%(axis)s.tn>] * 1.5]
  G38.8 %(axis)s[#<%(axis)s.hd> * -#<%(axis)s.lb>] F[#<%(axis)s.lv>]
  G38.6 %(axis)s[#<%(axis)s.hd> * #<%(axis)s.lb> * 1.5]
  G0 %(axis)s[#<%(axis)s.hd> * -#<%(axis)s.zb> + #<%(axis)sp>]
  G28.3 %(axis)s[#<%(axis)s.hp>]
'''

# Set axis unhomed
# Seek closed (home_dir * (travel_max - travel_min) * 1.5) at search_vel
# Seek open (home_dir * -latch_backoff) at latch_vel
# Seek closed (home_dir * latch_backoff * 1.5) at latch_vel
# Rapid to (home_dir * -(zero_backoff + switched_position))
# Set axis homed and home_position


class AVR():
    def __init__(self, ctrl):
        self.ctrl = ctrl

        self.vars = {}
        self.stream = None
        self.queue = deque()
        self.in_buf = ''
        self.command = None
        self.lcd_page = ctrl.lcd.add_new_page()
        self.install_page = True

        try:
            self.sp = serial.Serial(ctrl.args.serial, ctrl.args.baud,
                                    rtscts = 1, timeout = 0, write_timeout = 0)
            self.sp.nonblocking()

        except Exception as e:
            self.sp = None
            log.warning('Failed to open serial port: %s', e)

        if self.sp is not None:
            ctrl.ioloop.add_handler(self.sp, self.serial_handler,
                                    ctrl.ioloop.READ)

        self.i2c_addr = ctrl.args.avr_addr


    def _start_sending_gcode(self, path):
        if self.stream is not None:
            raise Exception('Busy, cannot start new GCode file')

        log.info('Running ' + path)
        self.stream = bbctrl.Planner(self.ctrl, path)
        self.set_write(True)


    def _stop_sending_gcode(self):
        if self.stream is not None:
            self.stream.reset()
            self.stream = None


    def connect(self):
        try:
            # Reset AVR communication
            self.stop();
            self.ctrl.config.config_avr()
            self._restore_machine_state()

        except Exception as e:
            log.warning('Connect failed: %s', e)
            self.ctrl.ioloop.call_later(1, self.connect)


    def _i2c_command(self, cmd, byte = None, word = None):
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
                    log.error('AVR I2C communication failed, retrying: %s' % e)
                    time.sleep(0.1)
                    continue

                else:
                    log.error('AVR I2C communication failed: %s' % e)
                    raise


    def _restore_machine_state(self):
        for var in machine_state_vars:
            if var in self.vars:
                value = self.vars[var]
                if isinstance(value, str): value = '"' + value + '"'
                if isinstance(value, bool): value = int(value)

                self.set('', var, value)

        self.queue_command('$$') # Refresh all vars, must come after above


    def report(self): self._i2c_command(Cmd.REPORT)


    def load_next_command(self, cmd):
        log.info('< ' + json.dumps(cmd).strip('"'))
        self.command = bytes(cmd.strip() + '\n', 'utf-8')


    def set_write(self, enable):
        if self.sp is None: return

        flags = self.ctrl.ioloop.READ
        if enable: flags |= self.ctrl.ioloop.WRITE
        self.ctrl.ioloop.update_handler(self.sp, flags)


    def serial_handler(self, fd, events):
        try:
            if self.ctrl.ioloop.READ & events: self.serial_read()
            if self.ctrl.ioloop.WRITE & events: self.serial_write()
        except Exception as e:
            log.error('Serial handler error: %s', e)


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
        if len(self.queue): self.load_next_command(self.queue.popleft())

        # Load next GCode command, if running or paused
        elif self.stream is not None:
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

        update = {}

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
                    log.error('%s, data: %s', e, line)
                    continue

                update.update(msg)
                log.debug(line)

                # Don't overwrite duplicate `msg`
                if 'msg' in msg: break

        if update:
            if 'firmware' in update:
                log.error('AVR rebooted')
                self.connect()

            if 'x' in update and update['x'] == 'ESTOPPED':
                self._stop_sending_gcode()

            self.vars.update(update)

            if self.stream is not None:
                self.stream.update(update)
                if not self.stream.is_running():
                    self.stream = None

            try:
                self._update_lcd(update)

                if self.install_page:
                    self.install_page = False
                    self.ctrl.lcd.set_current_page(self.lcd_page.id)

            except Exception as e:
                log.error('Updating LCD: %s', e)

            try:
                self.ctrl.web.broadcast(update)
            except Exception as e:
                log.error('Updating Web: %s', e)


    def _find_motor(self, axis):
        for motor in range(6):
            if not ('%dan' % motor) in self.vars: continue
            motor_axis = 'xyzabc'[self.vars['%dan' % motor]]
            if motor_axis == axis.lower() and self.vars['%dpm' % motor]:
                return motor


    def _is_axis_homed(self, axis):
        motor = self._find_motor(axis)
        if axis is None: return False
        return self.vars['%dh' % motor]


    def _update_lcd(self, msg):
        if 'x' in msg: self.lcd_page.text('%-9s' % self.vars['x'], 0, 0)

        # Show enabled axes
        row = 0
        for axis in 'xyzabc':
            motor = self._find_motor(axis)
            if motor is not None:
                if (axis + 'p') in msg:
                    self.lcd_page.text('% 10.3f%s' % (
                            msg[axis + 'p'], axis.upper()), 9, row)

                row += 1

        if 't' in msg:  self.lcd_page.text('%2uT'     % msg['t'],  6, 1)
        if 'u' in msg:  self.lcd_page.text('%-6s'     % msg['u'],  0, 1)
        if 'f' in msg:  self.lcd_page.text('%8uF'     % msg['f'],  0, 2)
        if 's' in msg:  self.lcd_page.text('%8dS'     % msg['s'],  0, 3)


    def queue_command(self, cmd):
        self.queue.append(cmd)
        self.set_write(True)


    def mdi(self, cmd):
        if self.stream is not None:
            raise Exception('Busy, cannot queue MDI command')

        self.queue_command(cmd)


    def jog(self, axes):
        if self.stream is not None: raise Exception('Busy, cannot jog')

        _axes = {}
        for i in range(len(axes)): _axes["xyzabc"[i]] = axes[i]

        self.queue_command(Cmd.jog(_axes))


    def set(self, index, code, value):
        self.queue_command('${}{}={}'.format(index, code, value))


    def home(self, axis, position = None):
        if self.stream is not None: raise Exception('Busy, cannot home')
        raise Exception('NYI') # TODO

        if position is not None:
            self.queue_command('G28.3 %c%f' % (axis, position))

        else:
            if axis is None: axes = 'zxyabc' # TODO This should be configurable
            else: axes = '%c' % axis

            for axis in axes:
                if not self.vars.get('%sch' % axis, 0): continue

                gcode = axis_homing_procedure % {'axis': axis}
                for line in gcode.splitlines():
                    self.queue_command(line.strip())


    def estop(self): self._i2c_command(Cmd.ESTOP)
    def clear(self): self._i2c_command(Cmd.CLEAR)


    def start(self, path):
        if self.stream is not None: raise Exception('Busy, cannot start file')
        if path: self._start_sending_gcode(path)


    def step(self, path):
        self._i2c_command(Cmd.STEP)
        if self.stream is None and path and self.vars.get('x', '') == 'READY':
            self._start_sending_gcode(path)


    def stop(self):
        self._i2c_command(Cmd.FLUSH)
        self._stop_sending_gcode()
        # Resume processing once current queue of GCode commands has flushed
        self.queue_command(Cmd.RESUME)


    def pause(self): self._i2c_command(Cmd.PAUSE, byte = 0)


    def unpause(self):
        if self.vars.get('x', '') != 'HOLDING' or self.stream is None: return

        self._i2c_command(Cmd.FLUSH)
        self.queue_command(Cmd.RESUME)
        self.stream.restart()
        self.set_write(True)
        self._i2c_command(Cmd.UNPAUSE)


    def optional_pause(self): self._i2c_command(Cmd.PAUSE, byte = 1)


    def set_position(self, axis, position):
        if self.stream is not None: raise Exception('Busy, cannot set position')
        if self._is_axis_homed('%c' % axis):
            raise Exception('NYI') # TODO
            self.queue_command('G92 %c%f' % (axis, position))
        else: self.queue_command('$%cp=%f' % (axis, position))
