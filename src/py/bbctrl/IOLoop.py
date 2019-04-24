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

import tornado.ioloop
import bbctrl


class CB(object):
    def __init__(self, ioloop, delay, cb, *args, **kwargs):
        self.ioloop = ioloop
        self.cb = cb

        io = ioloop.ioloop
        self.h = io.call_later(delay, self._cb, *args, **kwargs)

        ioloop.callbacks[self.h] = self

    def _cb(self, *args, **kwarg):
        del self.ioloop.callbacks[self.h]
        return self.cb(*args, **kwarg)


class IOLoop(object):
    READ = tornado.ioloop.IOLoop.READ
    WRITE = tornado.ioloop.IOLoop.WRITE
    ERROR = tornado.ioloop.IOLoop.ERROR


    def __init__(self, ioloop):
        self.ioloop = ioloop
        self.fds = set()
        self.handles = set()
        self.callbacks = {}


    def close(self):
        for fd in list(self.fds): self.ioloop.remove_handler(fd)
        for h in list(self.handles): self.ioloop.remove_timeout(h)
        for h in list(self.callbacks): self.ioloop.remove_timeout(h)


    def add_handler(self, fd, handler, events):
        self.ioloop.add_handler(fd, handler, events)
        if hasattr(fd, 'fileno'): fd = fd.fileno()
        self.fds.add(fd)


    def remove_handler(self, h):
        self.ioloop.remove_handler(h)
        if hasattr(h, 'fileno'): h = h.fileno()
        self.fds.remove(h)


    def update_handler(self, fd, events): self.ioloop.update_handler(fd, events)


    def call_later(self, delay, callback, *args, **kwargs):
        cb = CB(self, delay, callback, *args, **kwargs)
        return cb.h


    def remove_timeout(self, h):
        self.ioloop.remove_timeout(h)
        if h in self.handles: self.handles.remove(h)
        if h in self.callbacks: del self.callbacks[h]


    def add_callback(self, cb, *args, **kwargs):
        self.ioloop.add_callback(cb, *args, **kwargs)
