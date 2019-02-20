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


class IOLoop(object):
    READ = tornado.ioloop.IOLoop.READ
    WRITE = tornado.ioloop.IOLoop.WRITE
    ERROR = tornado.ioloop.IOLoop.ERROR


    def __init__(self, ioloop):
        self.ioloop = ioloop
        self.fds = set()
        self.handles = set()


    def close(self):
        for fd in self.fds: self.ioloop.remove_handler(fd)
        for h in self.handles: self.ioloop.remove_timeout(h)


    def add_handler(self, fd, handler, events):
        self.ioloop.add_handler(fd, handler, events)
        if hasattr(fd, 'fileno'): fd = fd.fileno()
        self.fds.add(fd)


    def remove_handler(self, fd):
        self.ioloop.remove_handler(fd)
        if hasattr(fd, 'fileno'): fd = fd.fileno()
        self.fds.remove(fd)


    def update_handler(self, fd, events): self.ioloop.update_handler(fd, events)


    def call_later(self, delay, callback, *args, **kwargs):
        h = self.ioloop.call_later(delay, callback, *args, **kwargs)
        self.handles.add(h)
        return h


    def remove_timeout(self, h):
        self.ioloop.remove_timeout(h)
        self.handles.remove(h)
