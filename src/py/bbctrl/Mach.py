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
from bbctrl.Comm import Comm
import bbctrl.Cmd as Cmd
from tornado.ioloop import PeriodicCallback

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


def overrides(interface_class):
    def overrider(method):
        if not method.__name__ in dir(interface_class):
            log.warning('%s does not override %s' % (
                method.__name__, interface_class.__name__))

        return method

    return overrider


class Mach(Comm):
    def __init__(self, ctrl):
        super().__init__(ctrl)

        self.ctrl = ctrl
        self.planner = bbctrl.Planner(ctrl)

        ctrl.state.set('cycle', 'idle')
        PeriodicCallback(self._update_cycle, 1000, ctrl.ioloop).start()

        ctrl.state.add_listener(self._update)

        super().reboot()


    def _get_state(self): return self.ctrl.state.get('xx', '')
    def _get_cycle(self): return self.ctrl.state.get('cycle')


    def _begin_cycle(self, cycle):
        current = self._get_cycle()
        if current == cycle: return # No change

        if current == 'idle':
            self.planner.update_position()
            self.ctrl.state.set('cycle', cycle)

        else:
            raise Exception('Cannot enter %s cycle while in %s cycle' %
                            (cycle, current))


    def _update_cycle(self):
        if (self._get_cycle() != 'idle' and self._get_state() == 'READY' and
            not self.planner.is_busy() and not super().is_active()):
            self.ctrl.state.set('cycle', 'idle')


    def _update(self, update):
        # Handle EStop
        if update.get('xx', '') == 'ESTOPPED': self.planner.reset()

        # Update cycle now, if it has changed
        self._update_cycle()

        if (('xx' in update or 'pr' in update) and
            self.ctrl.state.get('xx', '') == 'HOLDING'):
            pause_reason = self.ctrl.state.get('pr', '')

            # Continue after seek hold
            if (pause_reason == 'Switch found' and
                self.planner.is_synchronizing()):
                self._unpause()

            # Continue after stop hold
            if pause_reason == 'User stop':
                self.planner.stop()
                self.planner.update_position()
                self.ctrl.state.set('line', 0)
                self._unpause()


    def _unpause(self):
        pause_reason = self.ctrl.state.get('pr', '')
        log.info('Unpause: ' + pause_reason)

        if pause_reason in ['User pause', 'Switch found']:
            self.planner.restart()

        if pause_reason in ['User pause', 'User stop', 'Switch found']:
            super().i2c_command(Cmd.FLUSH)
            super().resume()


        super().i2c_command(Cmd.UNPAUSE)


    def _reset(self):
        self.planner.reset()
        self.ctrl.state.reset()


    def _i2c_block(self, block):
        super().i2c_command(block[0], block = block[1:])


    def _i2c_set(self, name, value): self._i2c_block(Cmd.set(name, value))


    @overrides(Comm)
    def comm_next(self):
        if self.planner.is_running(): return self.planner.next()


    @overrides(Comm)
    def comm_error(self): self._reset()


    @overrides(Comm)
    def connect(self):
        self._reset()
        super().connect()


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


    def mdi(self, cmd, with_limits = True):
        if not len(cmd): return
        if   cmd[0] == '$':  self._query_var(cmd)
        elif cmd[0] == '\\': super().queue_command(cmd[1:])
        else:
            self._begin_cycle('mdi')
            self.planner.mdi(cmd, with_limits)
            super().resume()


    def set(self, code, value):
        super().queue_command('${}={}'.format(code, value))


    def jog(self, axes):
        self._begin_cycle('jogging')
        super().queue_command(Cmd.jog(axes))


    def home(self, axis, position = None):
        state = self.ctrl.state

        if position is not None: self.mdi('G28.3 %c%f' % (axis, position))

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
                self.planner.mdi(axis_homing_procedure % {'axis': axis}, False)


    def unhome(self, axis): self.mdi('G28.2 %c0' % axis)


    def estop(self): super().estop()


    def clear(self):
        if self._get_state() == 'ESTOPPED':
            self._reset()
            super().clear()


    def select(self, path):
        if self.ctrl.state.get('selected', '') == path: return

        if self._get_cycle() != 'idle':
            raise Exception('Cannot select file during ' + self._get_cycle())

        self.ctrl.state.set('selected', path)


    def start(self):
        self._begin_cycle('running')
        self.planner.load('upload/' + self.ctrl.state.get('selected'))
        super().resume()


    def step(self):
        raise Exception('NYI') # TODO
        if self._get_cycle() != 'running': self.start()
        else: super().i2c_command(Cmd.UNPAUSE)


    def stop(self):
        if self._get_cycle() == 'idle': self._begin_cycle('running')
        super().i2c_command(Cmd.STOP)


    def pause(self): super().pause()


    def unpause(self):
        if self._get_state() != 'HOLDING': return

        pause_reason = self.ctrl.state.get('pr', '')
        if pause_reason in ['User pause', 'Program pause']:
            self._unpause()


    def optional_pause(self, enable = True):
        super().queue_command('$op=%d' % enable)


    def set_position(self, axis, position):
        axis = axis.lower()

        if self.ctrl.state.is_axis_homed(axis):
            self.mdi('G92 %s%f' % (axis, position))

        else:
            if self._get_cycle() not in ['idle', 'mdi']:
                raise Exception('Cannot zero position during ' +
                                self._get_cycle())

            self._begin_cycle('mdi')
            self.planner.set_position({axis: position})
            super().queue_command(Cmd.set_axis(axis, position))


    def override_feed(self, override):
        self._i2c_set('fo', int(1000 * override))


    def override_speed(self, override):
        self._i2c_set('so', int(1000 * override))


    def modbus_read(self, addr): self._i2c_block(Cmd.modbus_read(addr))


    def modbus_write(self, addr, value):
        self._i2c_block(Cmd.modbus_write(addr, value))
