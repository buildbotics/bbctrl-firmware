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
import time
import bbctrl

log = logging.getLogger('PlanTimer')


class PlanTimer(object):
    def __init__(self, ctrl):
        self.ctrl = ctrl

        self.reset()
        self._report()

        self.ctrl.state.set('plan_time', 0)
        ctrl.state.add_listener(self._update)


    def reset(self):
        self.plan_time = 0
        self.move_start = None
        self.hold_start = None
        self.plan_times = None
        self.plan_index = 0


    def _report(self):
        if (self.plan_times is not None and
            self.plan_index < len(self.plan_times) and
            self.move_start is not None):
            state = self.ctrl.state.get('xx', '')

            if state in ['STOPPING', 'RUNNING']:
                t = self.plan_time
                delta = time.time() - self.move_start
                nextT = self.plan_times[self.plan_index][1]
                if t + delta < nextT: t += delta
                else: t = nextT

                self.ctrl.state.set('plan_time', round(t))

        self.timer = self.ctrl.ioloop.call_later(1, self._report)


    def _update(self, update):
        # Check state
        if 'xx' in update:
            state = update['xx']

            if state in ['READY', 'ESTOPPED']:
                self.ctrl.state.set('plan_time', 0)
                self.reset()

            elif state == 'HOLDING': self.hold_start = time.time()
            elif (state == 'RUNNING' and self.hold_start is not None and
                  self.move_start is not None):
                self.move_start += time.time() - self.hold_start
                self.hold_start = None

        # Get plan times
        if self.plan_times is None or 'selected' in update:
            active_plan = self.ctrl.state.get('selected', '')

            if active_plan:
                plan = self.ctrl.preplanner.get_plan(active_plan)

                if plan is not None and plan.done():
                    self.reset()
                    self.plan_times = plan.result()[1]

        # Get plan time for current id
        if self.plan_times is not None and 'id' in update:
            currentID = update['id']

            while self.plan_index < len(self.plan_times):
                id, t = self.plan_times[self.plan_index]
                if id <= currentID: self.move_start = time.time()
                if currentID <= id: break
                self.plan_time = t
                self.plan_index += 1
