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

import logging

import bbctrl
import bbctrl.Cmd as Cmd
import bbctrl.Comm

log = logging.getLogger('Mach')


# Axis homing procedure:
#
#   Mark axis unhomed
#   Seek closed (home_dir * (travel_max - travel_min) * 1.5) at search_velocity
#   Seek open (home_dir * -latch_backoff) at latch_vel
#   Seek closed (home_dir * latch_backoff * 1.5) at latch_vel
#   Rapid to (home_dir * -zero_backoff + position)
#   Mark axis homed and set absolute position

axis_homing_procedure = '''
  G28.2 %(axis)s0 F[#<_%(axis)s_sv> * 1000]
  G38.6 %(axis)s[#<_%(axis)s_hd> * [#<_%(axis)s_tm> - #<_%(axis)s_tn>] * 1.5]
  G38.8 %(axis)s[#<_%(axis)s_hd> * -#<_%(axis)s_lb>] F[#<_%(axis)s_lv> * 1000]
  G38.6 %(axis)s[#<_%(axis)s_hd> * #<_%(axis)s_lb> * 1.5]
  G91 G0 G53 %(axis)s[#<_%(axis)s_hd> * -#<_%(axis)s_zb>]
  G90 G28.3 %(axis)s[#<_%(axis)s_hp>]
'''



class Mach():
    def __init__(self, ctrl):
        self.ctrl = ctrl
        self.comm = bbctrl.Comm(ctrl)
        self.stopping = False

        ctrl.state.add_listener(self._update)

        self.comm.queue_command(Cmd.REBOOT)


    def _is_busy(self): return self.ctrl.planner.is_running()


    def _update(self, update):
        if self.stopping and 'xx' in update and update['xx'] == 'HOLDING':
            self.comm.stop_sending_gcode()
            # Resume once current queue of GCode commands has flushed
            self.comm.i2c_command(Cmd.FLUSH)
            self.comm.queue_command(Cmd.RESUME)
            self.stopping = False

        # Automatically unpause on seek hold
        if self.ctrl.state.get('xx', '') == 'HOLDING' and \
                self.ctrl.state.get('pr', '') == 'Switch found' and \
                self.ctrl.planner.is_synchronizing():
            self.ctrl.mach.unpause()


    def set(self, code, value):
        self.comm.queue_command('${}={}'.format(code, value))


    def mdi(self, cmd):
        if len(cmd) and cmd[0] == '$':
            equal = cmd.find('=')
            if equal == -1:
                log.info('%s=%s' % (cmd, self.ctrl.state.get(cmd[1:])))

            else:
                name, value = cmd[1:equal], cmd[equal + 1:]

                if value.lower() == 'true': value = True
                elif value.lower() == 'false': value = False
                else:
                    try:
                        value = float(value)
                    except: pass

                self.ctrl.state.config(name, value)

        elif len(cmd) and cmd[0] == '\\': self.comm.queue_command(cmd[1:])

        else:
            self.ctrl.planner.mdi(cmd)
            self.comm.set_write(True)


    def jog(self, axes):
        if self._is_busy(): raise Exception('Busy, cannot jog')
        self.comm.queue_command(Cmd.jog(axes))


    def home(self, axis, position = None):
        if self._is_busy(): raise Exception('Busy, cannot home')

        if position is not None:
            self.mdi('G28.3 %c%f' % (axis, position))

        else:
            if axis is None: axes = 'zxyabc' # TODO This should be configurable
            else: axes = '%c' % axis

            for axis in axes:
                if not self.ctrl.state.axis_can_home(axis):
                    log.info('Cannot home ' + axis)
                    continue

                log.info('Homing %s axis' % axis)
                self.mdi(axis_homing_procedure % {'axis': axis})


    def estop(self): self.comm.i2c_command(Cmd.ESTOP)
    def clear(self): self.comm.i2c_command(Cmd.CLEAR)
    def start(self, path): self.comm.start_sending_gcode(path)


    def step(self, path):
        self.comm.i2c_command(Cmd.STEP)
        if not self._is_busy() and path and \
                self.ctrl.state.get('xx', '') == 'READY':
            self.comm.start_sending_gcode(path)


    def stop(self):
        self.pause()
        self.stopping = True


    def pause(self): self.comm.i2c_command(Cmd.PAUSE, byte = 0)


    def unpause(self):
        if self.ctrl.state.get('xx', '') != 'HOLDING': return

        self.comm.i2c_command(Cmd.FLUSH)
        self.comm.queue_command(Cmd.RESUME)
        self.ctrl.planner.restart()
        self.comm.set_write(True)
        self.comm.i2c_command(Cmd.UNPAUSE)


    def optional_pause(self): self.comm.i2c_command(Cmd.PAUSE, byte = 1)


    def set_position(self, axis, position):
        if self._is_busy(): raise Exception('Busy, cannot set position')

        if self.ctrl.state.is_axis_homed('%c' % axis):
            self.mdi('G92 %c%f' % (axis, position))
        else: self.comm.queue_command('$%cp=%f' % (axis, position))
