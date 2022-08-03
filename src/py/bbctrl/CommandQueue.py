################################################################################
#                                                                              #
#                 This file is part of the Buildbotics firmware.               #
#                                                                              #
#        Copyright (c) 2015 - 2021, Buildbotics LLC, All rights reserved.      #
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

from . import util
from .Log import *

__all__ = ['CommandQueue']


class CommandQueue():
    def __init__(self, ctrl):
        self.log = ctrl.log.get('CmdQ')
        self.log.set_level(Log.WARNING)

        self.lastEnqueueID = 0
        self.releaseID = 0
        self.q = deque()


    def is_active(self): return len(self.q)


    def clear(self):
        self.lastEnqueueID = 0
        self.releaseID = 0
        self.q.clear()


    def enqueue(self, id, cb, *args, **kwargs):
        self.log.info('add(#%d) releaseID=%d', id, self.releaseID)
        self.lastEnqueueID = id
        self.q.append([id, cb, args, kwargs])
        self._release()


    def _release(self):
        while len(self.q):
            id, cb, args, kwargs = self.q[0]

            # Execute commands <= releaseID
            if util.id16_less(self.releaseID, id): return

            self.log.info('releasing id=%d' % id)
            self.q.popleft()

            try:
                if cb is not None: cb(*args, **kwargs)
            except Exception:
                self.log.exception('During command queue callback')



    def release(self, id):
        if id and not util.id16_less(self.releaseID, id):
            self.log.debug('id out of order %d <= %d' % (id, self.releaseID))
        self.releaseID = id

        self._release()
