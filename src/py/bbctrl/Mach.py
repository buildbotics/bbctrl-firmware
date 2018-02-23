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
        self.planner = bbctrl.Planner(ctrl)
        self.comm = bbctrl.Comm(ctrl, self._comm_next, self._comm_connect)
        self.stopping = False

        ctrl.state.set('cycle', 'idle')
        ctrl.state.add_listener(self._update)

        self.comm.queue_command(Cmd.REBOOT)


    def _get_cycle(self): return self.ctrl.state.get('cycle')


    def _begin_cycle(self, cycle):
        current = self._get_cycle()

        if current == 'idle':
            self.planner.update_position()
            self.ctrl.state.set('cycle', cycle)

        elif current != cycle:
            raise Exception('Cannot enter %s cycle during %s' %
                            (cycle, current))


    def _update(self, update):
        state = self.ctrl.state.get('xx', '')

        # Handle EStop
        if 'xx' in update and state == 'ESTOPPED':
            self._stop_sending_gcode()

        # Handle stop
        if self.stopping and 'xx' in update and state == 'HOLDING':
            self._stop_sending_gcode()
            # Resume once current queue of GCode commands has flushed
            self.comm.i2c_command(Cmd.FLUSH)
            self.comm.queue_command(Cmd.RESUME)
            self.ctrl.state.set('line', 0)
            self.stopping = False

        # Update cycle
        if (self._get_cycle() != 'idle' and not self.planner.is_busy() and
            not self.comm.is_active() and state == 'READY'):
            self.ctrl.state.set('cycle', 'idle')

        # Automatically unpause on seek hold
        if (state == 'HOLDING' and
            self.ctrl.state.get('pr', '') == 'Switch found' and
            self.planner.is_synchronizing()):
            self.unpause()


    def _comm_next(self):
        if self.planner.is_running(): return self.planner.next()


    def _comm_connect(self): self._stop_sending_gcode()


    def _start_sending_gcode(self, path):
        self.planner.load('upload/' + path)
        self.comm.set_write(True)


    def _stop_sending_gcode(self): self.planner.reset()


    def _query_var(self, cmd):
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


    def mdi(self, cmd):
        if not len(cmd): return
        if   cmd[0] == '$':  self._query_var(cmd)
        elif cmd[0] == '\\': self.comm.queue_command(cmd[1:])
        else:
            self._begin_cycle('mdi')
            self.planner.mdi(cmd)
            self.comm.set_write(True)


    def set(self, code, value):
        self.comm.queue_command('${}={}'.format(code, value))


    def jog(self, axes):
        self._begin_cycle('jogging')
        self.comm.queue_command(Cmd.jog(axes))


    def home(self, axis, position = None):
        if position is not None:
            self.mdi('G28.3 %c%f' % (axis, position))

        else:
            self._begin_cycle('homing')

            if axis is None: axes = 'zxyabc' # TODO This should be configurable
            else: axes = '%c' % axis

            for axis in axes:
                if not self.ctrl.state.axis_can_home(axis):
                    log.warning('Cannot home ' + axis)
                    continue

                log.info('Homing %s axis' % axis)
                self.planner.mdi(axis_homing_procedure % {'axis': axis})
                self.comm.set_write(True)


    def estop(self): self.comm.i2c_command(Cmd.ESTOP)
    def clear(self): self.comm.i2c_command(Cmd.CLEAR)


    def select(self, path):
        if self.ctrl.state.get('selected', '') == path: return

        if self._get_cycle() != 'idle':
            raise Exception('Cannot select file during ' + self._get_cycle())

        self.ctrl.state.set('selected', path)


    def start(self):
        self._begin_cycle('running')
        self._start_sending_gcode(self.ctrl.state.get('selected'))


    def step(self):
        raise Exception('NYI') # TODO
        self.comm.i2c_command(Cmd.STEP)
        if self._get_cycle() != 'running': self.start()


    def stop(self):
        self.pause()
        self.stopping = True


    def pause(self): self.comm.i2c_command(Cmd.PAUSE, byte = 0)


    def unpause(self):
        if self.ctrl.state.get('xx', '') != 'HOLDING': return

        self.comm.i2c_command(Cmd.FLUSH)
        self.comm.queue_command(Cmd.RESUME)
        self.planner.restart()
        self.comm.set_write(True)
        self.comm.i2c_command(Cmd.UNPAUSE)


    def optional_pause(self):
        if self._get_cycle() == 'running':
            self.comm.i2c_command(Cmd.PAUSE, byte = 1)


    def set_position(self, axis, position):
        axis = axis.lower()

        if self.ctrl.state.is_axis_homed(axis):
            self.mdi('G92 %s%f' % (axis, position))

        else:
            if self._get_cycle() != 'idle':
                raise Exception('Cannot zero position during ' +
                                self._get_cycle())

            self._begin_cycle('mdi')
            self.planner.set_position({axis: position})
            self.comm.queue_command(Cmd.set_axis(axis, position))
