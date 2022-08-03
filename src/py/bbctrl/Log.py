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

import os
import sys
import io
import traceback
from inspect import getframeinfo, stack

from . import util

__all__ = ['Log']


class Logger(object):
    def __init__(self, log, name, level):
        self.log = log
        self.name = name
        self.level = level


    def set_level(self, level): self.level = level
    def _enabled(self, level): return self.level <= level and level <= Log.ERROR


    def _find_caller(self):
        f = sys._getframe()
        if f is not None: f = f.f_back

        while hasattr(f, 'f_code'):
            co = f.f_code

            filename = os.path.normcase(co.co_filename)
            if filename == Logger.__init__.__code__.co_filename:
                f = f.f_back
                continue

            return co.co_filename, f.f_lineno, co.co_name

        return '(unknown file)', 0, '(unknown function)'


    def _log(self, level, msg, *args, **kwargs):
        if not self._enabled(level): return

        if not 'where' in kwargs:
            filename, line, func = self._find_caller()
            kwargs['where'] = '%s:%d' % (os.path.basename(filename), line)

        if len(args): msg %= args

        self.log._log(msg, level = level, prefix = self.name, **kwargs)


    def debug  (self, *args, **kwargs): self._log(Log.DEBUG,   *args, **kwargs)
    def message(self, *args, **kwargs): self._log(Log.MESSAGE, *args, **kwargs)
    def info   (self, *args, **kwargs): self._log(Log.INFO,    *args, **kwargs)
    def warning(self, *args, **kwargs): self._log(Log.WARNING, *args, **kwargs)
    def error  (self, *args, **kwargs): self._log(Log.ERROR,   *args, **kwargs)


    def exception(self, *args, **kwargs):
        msg = traceback.format_exc()
        if len(args): msg = args[0] % args[1:] + '\n' + msg
        self._log(Log.ERROR, msg, **kwargs)


class Log(object):
    DEBUG    = 0
    INFO     = 1
    MESSAGE  = 2
    WARNING  = 3
    ERROR    = 4

    level_names = 'debug info message warning error'.split()


    def __init__(self, args, ioloop, path):
        self.path = path
        self.listeners = []
        self.loggers = {}

        self.level = self.DEBUG if args.verbose else self.INFO

        # Open log, rotate if necessary
        self.f = None
        self._open()

        # Log header
        self._log('Log started v%s' % util.get_version())
        self._log_time(ioloop)


    def get_path(self): return self.path

    def add_listener(self, listener): self.listeners.append(listener)
    def remove_listener(self, listener): self.listeners.remove(listener)


    def get(self, name, level = None):
        if not name in self.loggers:
            self.loggers[name] = Logger(self, name, self.level)
        return self.loggers[name]


    def _log_time(self, ioloop):
        self._log(util.timestamp())
        ioloop.call_later(60 * 60, self._log_time, ioloop)


    def broadcast(self, msg):
        for listener in self.listeners: listener(msg)


    def _log(self, msg, level = INFO, prefix = '', where = None, time = False):
        if not msg: return

        hdr = '%s:%s:' % ('DIMWE'[level], prefix)
        if time: hdr += util.timestamp() + ':'
        s = hdr + ('\n' + hdr).join(msg.split('\n'))

        if self.f is not None:
            if 1e22 <= self.bytes_written + len(s) + 1: self._open()
            self.f.write(s + '\n')
            self.f.flush()
            self.bytes_written += len(s) + 1

        print(s)

        # Broadcast to log listeners
        if level == self.INFO: return

        msg = dict(level = self.level_names[level], source = prefix, msg = msg)
        if where is not None: msg['where'] = where

        self.broadcast(dict(log = msg))


    def _open(self):
        if self.path is None: return
        if self.f is not None: self.f.close()
        self._rotate(self.path)
        self.f = open(self.path, 'w')
        self.bytes_written = 0


    def _rotate(self, path, n = None):
        fullpath = '%s.%d' % (path, n) if n is not None else path
        nextN = (0 if n is None else n) + 1

        if os.path.exists(fullpath):
            if n == 16: os.unlink(fullpath)
            else:
                self._rotate(path, nextN)
                os.rename(fullpath, '%s.%d' % (path, nextN))
