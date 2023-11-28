################################################################################
#                                                                              #
#                 This file is part of the Buildbotics firmware.               #
#                                                                              #
#        Copyright (c) 2015 - 2023, Buildbotics LLC, All rights reserved.      #
#                                                                              #
#         This Source describes Open Hardware and is licensed under the        #
#                                 CERN-OHL-S v2.                               #
#                                                                              #
#         You may redistribute and modify this Source and make products        #
#    using it under the terms of the CERN-OHL-S v2 (https:/cern.ch/cern-ohl).  #
#           This Source is distributed WITHOUT ANY EXPRESS OR IMPLIED          #
#    WARRANTY, INCLUDING OF MERCHANTABILITY, SATISFACTORY QUALITY AND FITNESS  #
#     FOR A PARTICULAR PURPOSE. Please see the CERN-OHL-S v2 for applicable    #
#                                  conditions.                                 #
#                                                                              #
#                Source location: https://github.com/buildbotics               #
#                                                                              #
#      As per CERN-OHL-S v2 section 4, should You produce hardware based on    #
#    these sources, You must maintain the Source Location clearly visible on   #
#    the external case of the CNC Controller or other product you make using   #
#                                  this Source.                                #
#                                                                              #
#                For more information, email info@buildbotics.com              #
#                                                                              #
################################################################################

from collections import deque

from . import Cmd
from . import util
from .Comm import *
from .Planner import *
from .ProgramMDI import *
from .ProgramJog import *
from .ProgramHome import *
from .ProgramFile import *
from .ProgramSetPosition import *

__all__ = ['Mach']


motor_fault_error = '''\
Motor %d driver fault.  A potentially damaging electrical condition was \
detected and the motor driver was shutdown.  Please power down the controller \
and check your motor cabling. See the "Motor Faults" table on the "Indicators" \
for more information.\
'''


class Mach(Comm):
    def __init__(self, ctrl, avr):
        super().__init__(ctrl, avr)

        self.ctrl = ctrl
        self.mlog = self.ctrl.log.get('Mach')

        self.planner = Planner(ctrl)
        self.programs = deque()
        self.unpausing = False
        self.stopping = False
        self.next_jog_id = 1

        ctrl.state.set('cycle', 'idle')
        ctrl.state.add_listener(self._update)

        super().reboot()


    def _get_state(self): return self.ctrl.state.get('xx', '')
    def _is_estopped(self): return self._get_state() == 'ESTOPPED'
    def _is_holding(self): return self._get_state() == 'HOLDING'
    def _is_ready(self): return self._get_state() == 'READY'
    def _get_pause_reason(self): return self.ctrl.state.get('pr', '')
    def _get_cycle(self): return self.ctrl.state.get('cycle', 'idle')
    def _set_cycle(self, cycle): return self.ctrl.state.set('cycle', cycle)


    def _is_paused(self):
        if not self._is_holding() or self.unpausing: return False
        return self._get_pause_reason() in (
            'User pause', 'Program pause', 'Optional pause')


    def _update(self, update):
        # Detect motor faults
        for motor in range(4):
            key = '%ddf' % motor
            if key in update and update[key] & 0x1f:
                self.mlog.error(motor_fault_error % motor)

        # Get state
        state_changed = 'xc' in update
        state = self._get_state()

        # Handle EStop
        if state_changed and state == 'ESTOPPED':
            self.programs.clear()
            self.planner.reset(False)
            self.enter_estop()

        # Jog ID
        if 'jd' in update: self.end_jog(update['jd'])

        # Planner stop
        if state == 'READY' and self.stopping:
            self.planner.stop()
            self.ctrl.state.set('line', 0)
            self.stopping = False

        # Unpause sync
        if state_changed and state != 'HOLDING': self.unpausing = False

        # Entering HOLDING state
        if state_changed and state == 'HOLDING':
            # Always flush queue after pause
            super().i2c_command(Cmd.FLUSH)
            super().resume()

        # Automatically unpause after seek or stop hold
        # Must be after holding commands above
        op = self.ctrl.state.get('optional_pause', False)
        pr = self._get_pause_reason()
        if ((state_changed or 'pr' in update) and self._is_holding()):
            if ((pr in ('Switch found', 'User stop') or
                 (pr == 'Optional pause' and not op))):
                self._unpause()

            if pr == 'Switch not found':
                self.mlog.error(pr)
                self.stop()


    def _unpause(self):
        pause_reason = self._get_pause_reason()
        self.mlog.info('Unpause: ' + pause_reason)

        if pause_reason == 'User stop':
            self.planner.stop()
            self.ctrl.state.set('line', 0)

        else: self.planner.restart()

        super().i2c_command(Cmd.UNPAUSE)
        self.unpausing = True


    def _update_cycle(self):
        if len(self.programs): self._set_cycle(self.programs[0].status)
        else: self._set_cycle('idle')


    def _run(self, program):
        # Check if this program can be run now
        if len(self.programs):
            last = self.programs[-1].status

            if last != program.status:
                if last == 'jogging' or program.status == 'jogging':
                    raise Exception('Cannot enter %s cycle while in %s cycle' %
                                    (program.status, last))

        # Run program
        if program.start(self, self.planner):
            self.programs.append(program)
            self._update_cycle()


    def end(self, program):
        if not len(self.programs) or not program is self.programs[0]:
            raise Exception('End program mismatch: ' + program.status)

        self.programs.popleft()
        self._update_cycle()


    def end_jog(self, id):
        if not id: return # Ignore ID 0

        if (not len(self.programs) or
            self.programs[0].status != 'jogging' or
            id < self.programs[0].id):
            raise Exception('End jog mismatch: ' + id)

        while (len(self.programs) and
               not util.id16_less(id, self.programs[0].id)):
            self.end(self.programs[0])


    def estop(self):
        self.planner.reset(False)
        self.programs.clear()
        super().estop()


    def clear(self):
        if self._is_estopped():
            self.planner.reset()
            super().clear()


    def override_feed(self,  override): super().i2c_set('fo', float(override))
    def override_speed(self, override): super().i2c_set('so', float(override))


    def set(self, code, value):
        super().queue_command('${}={}'.format(code, value))


    def step(self): raise Exception('NYI') # TODO


    def stop(self):
        if self._get_cycle() != 'jogging': self.stopping = True
        super().i2c_command(Cmd.STOP)


    def pause(self): super().pause()


    def unpause(self):
        if self._is_paused():
            self.ctrl.state.set('optional_pause', False)
            self._unpause()


    def optional_pause(self, enable = True):
        self.ctrl.state.set('optional_pause', enable)


    def mdi(self, cmd, with_limits = True):
        self._run(ProgramMDI(self.ctrl, cmd, with_limits))


    def jog(self, axes):
        self._run(ProgramJog(self.ctrl, self.next_jog_id, axes))

        self.next_jog_id += 1
        if 0xffff < self.next_jog_id: self.next_jog_id = 1


    def home(self, axis, position = None):
        self._run(ProgramHome(self.ctrl, axis, position))


    def unhome(self, axis): self.mdi('G28.2 %c0' % axis)


    def start(self, path): self._run(ProgramFile(self.ctrl, path))


    def set_position(self, axis, position):
        self._run(ProgramSetPosition(self.ctrl, axis, position))


    # Comm methods
    def comm_next(self):
        cmd = None

        if self.planner.is_running() and not self._is_holding():
            cmd = self.planner.next()

        return cmd


    def comm_error(self): self.planner.reset()


    def connect(self):
        self.planner.reset()
        super().connect()
