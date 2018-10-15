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
        self.plan_times = None

        self.reset()
        self.ctrl.state.set('plan_time', 0)
        ctrl.state.add_listener(self._update)
        self._report()


    def reset(self):
        self.plan_time = 0
        self.move_start = None
        self.hold_start = None
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

        self.ctrl.ioloop.call_later(1, self._report)


    def _update_state(self, state):
        if state in ['READY', 'ESTOPPED']:
            self.ctrl.state.set('plan_time', 0)
            self.reset()

        elif state == 'HOLDING': self.hold_start = time.time()
        elif (state == 'RUNNING' and self.hold_start is not None and
              self.move_start is not None):
            self.move_start += time.time() - self.hold_start
            self.hold_start = None


    def _update_times(self, filename):
        if not filename: return
        future = self.ctrl.preplanner.get_plan(filename)

        def cb(future):
            if (filename != self.ctrl.state.get('selected') or
                future.cancelled()): return

            self.reset()
            path, meta = future.result()
            self.plan_times = meta['times']

        self.ctrl.ioloop.add_future(future, cb)


    def _update_time(self, currentID):
        if self.plan_times is None: return

        while self.plan_index < len(self.plan_times):
            id, t = self.plan_times[self.plan_index]
            if id <= currentID: self.move_start = time.time()
            if currentID <= id: break
            self.plan_time = t
            self.plan_index += 1


    def _update(self, update):
        # Check state
        if 'xx' in update: self._update_state(update['xx'])

        # Get plan times
        if 'selected' in update: self._update_times(update['selected'])

        # Get plan time for current id
        if 'id' in update: self._update_time(update['id'])
