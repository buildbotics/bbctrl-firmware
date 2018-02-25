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
        self.update_timer = None

        ctrl.state.set('cycle', 'idle')
        ctrl.state.add_listener(self._update)

        self.comm.reboot()


    def _get_state(self): return self.ctrl.state.get('xx', '')
    def _get_cycle(self): return self.ctrl.state.get('cycle')


    def _begin_cycle(self, cycle):
        current = self._get_cycle()

        if current == 'idle':
            self.planner.update_position()
            self.ctrl.state.set('cycle', cycle)

        elif current != cycle:
            raise Exception('Cannot enter %s cycle during %s' %
                            (cycle, current))


    def _update_cycle(self):
        # Cancel timer if set
        if self.update_timer is not None:
            self.ctrl.ioloop.remove_timeout(self.update_timer)
            self.update_timer = None

        # Check for idle state
        if self._get_cycle() != 'idle' and self._get_state() == 'READY':
            # Check again later if busy
            if self.planner.is_busy() or self.comm.is_active():
                self.ctrl.ioloop.call_later(0.5, self._update_cycle)

            else: self.ctrl.state.set('cycle', 'idle')


    def _update(self, update):
        state = self._get_state()

        # Handle EStop
        if 'xx' in update and state == 'ESTOPPED': self.planner.reset()

        # Update cycle
        self._update_cycle()

        # Continue after seek hold
        if (state == 'HOLDING' and self.planner.is_synchronizing() and
            self.ctrl.state.get('pr', '') == 'Switch found'):
            self.unpause()


    def _comm_next(self):
        if self.planner.is_running(): return self.planner.next()


    def _comm_connect(self):
        self.ctrl.state.reset()
        self.planner.reset()


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
            self.comm.resume()


    def set(self, code, value):
        self.comm.queue_command('${}={}'.format(code, value))


    def jog(self, axes):
        self._begin_cycle('jogging')
        self.comm.queue_command(Cmd.jog(axes))


    def home(self, axis, position = None):
        state = self.ctrl.state

        if position is not None:
            self.mdi('G28.3 %c%f' % (axis, position))

        else:
            self._begin_cycle('homing')

            if axis is None: axes = 'zxyabc' # TODO This should be configurable
            else: axes = '%c' % axis

            for axis in axes:
                # If this is not a request to home a specific axis and the
                # axis is disabled or in manual homing mode, don't show any
                # warnings
                if 1 < len(axes) and (
                        not state.is_axis_enabled(axis) or
                        state.axis_homing_mode(axis) == 'manual'):
                    continue

                # Error when axes cannot be homed
                reason = state.axis_home_fail_reason(axis)
                if reason is not None:
                    log.error('Cannot home %s axis: %s' % (
                        axis.upper(), reason))
                    continue

                # Home axis
                log.info('Homing %s axis' % axis)
                self.planner.mdi(axis_homing_procedure % {'axis': axis})
                self.comm.resume()


    def estop(self): self.comm.estop()


    def clear(self):
        if self._get_state() == 'ESTOPPED':
            self.ctrl.state.reset()
            self.comm.clear()


    def select(self, path):
        if self.ctrl.state.get('selected', '') == path: return

        if self._get_cycle() != 'idle':
            raise Exception('Cannot select file during ' + self._get_cycle())

        self.ctrl.state.set('selected', path)


    def start(self):
        self._begin_cycle('running')
        self.planner.load('upload/' + self.ctrl.state.get('selected'))
        self.comm.resume()


    def step(self):
        raise Exception('NYI') # TODO
        if self._get_cycle() != 'running': self.start()
        else: self.comm.i2c_command(Cmd.UNPAUSE)


    def stop(self):
        if self._get_cycle() == 'idle': self._begin_cycle('running')
        self.comm.i2c_command(Cmd.STOP)
        self.planner.stop()
        self.ctrl.state.set('line', 0)


    def pause(self): self.comm.pause()


    def unpause(self):
        if self._get_state() != 'HOLDING': return

        pause_reason = self.ctrl.state.get('pr', '')
        if pause_reason in ['User paused', 'Switch found']:
            self.planner.restart()
            self.comm.resume()

        self.comm.i2c_command(Cmd.UNPAUSE)


    def optional_pause(self):
        # TODO this could work better as a variable, i.e. $op=1
        if self._get_cycle() == 'running': self.comm.pause(True)


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
