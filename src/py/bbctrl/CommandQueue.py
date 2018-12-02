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
from collections import deque

log = logging.getLogger('CmdQ')
log.setLevel(logging.WARNING)


# 16-bit less with wrap around
def id_less(a, b): return (1 << 15) < (a - b) % ((1 << 16) - 1)


class CommandQueue():
    def __init__(self):
        self.lastEnqueueID = 0
        self.releaseID = 0
        self.q = deque()


    def is_active(self): return len(self.q)


    def clear(self):
        self.lastEnqueueID = 0
        self.releaseID = 0
        self.q.clear()


    def enqueue(self, id, cb, *args, **kwargs):
        log.info('add(#%d) releaseID=%d', id, self.releaseID)
        self.lastEnqueueID = id
        self.q.append([id, cb, args, kwargs])
        self._release()


    def _release(self):
        while len(self.q):
            id, cb, args, kwargs = self.q[0]

            # Execute commands <= releaseID
            if id_less(self.releaseID, id): return

            log.info('releasing id=%d' % id)
            self.q.popleft()

            try:
                if cb is not None: cb(*args, **kwargs)
            except Exception as e:
                log.exception('During command queue callback')



    def release(self, id):
        if id and not id_less(self.releaseID, id):
            log.debug('id out of order %d <= %d' % (id, self.releaseID))
        self.releaseID = id

        self._release()
