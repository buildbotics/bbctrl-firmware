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

import tornado.ioloop

__all__ = ['IOLoop']


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
    READ  = tornado.ioloop.IOLoop.READ
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


    def add_future(self, future, cb):
        self.ioloop.add_future(future, cb)
