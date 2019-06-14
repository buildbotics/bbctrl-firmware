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

import os
import sys
import io
import datetime
import traceback
import pkg_resources
from inspect import getframeinfo, stack
import bbctrl


DEBUG    = 0
INFO     = 1
WARNING  = 2
ERROR    = 3


def get_level_name(level): return 'debug info warning error'.split()[level]


# Get this file's name
_srcfile = os.path.normcase(get_level_name.__code__.co_filename)


class Logger(object):
    def __init__(self, log, name, level):
        self.log = log
        self.name = name
        self.level = level


    def set_level(self, level): self.level = level
    def _enabled(self, level): return self.level <= level and level <= ERROR


    def _find_caller(self):
        f = sys._getframe()
        if f is not None: f = f.f_back

        while hasattr(f, 'f_code'):
            co = f.f_code

            filename = os.path.normcase(co.co_filename)
            if filename == _srcfile:
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


    def debug  (self, *args, **kwargs): self._log(DEBUG,   *args, **kwargs)
    def info   (self, *args, **kwargs): self._log(INFO,    *args, **kwargs)
    def warning(self, *args, **kwargs): self._log(WARNING, *args, **kwargs)
    def error  (self, *args, **kwargs): self._log(ERROR,   *args, **kwargs)


    def exception(self, *args, **kwargs):
        msg = traceback.format_exc()
        if len(args): msg = args[0] % args[1:] + '\n' + msg
        self._log(ERROR, msg, **kwargs)


class Log(object):
    def __init__(self, args, ioloop, path):
        self.path = path
        self.listeners = []
        self.loggers = {}

        self.level = DEBUG if args.verbose else INFO

        # Open log, rotate if necessary
        self.f = None
        self._open()

        # Log header
        version = pkg_resources.require('bbctrl')[0].version
        self._log('Log started v%s' % version)
        self._log_time(ioloop)


    def get_path(self): return self.path

    def add_listener(self, listener): self.listeners.append(listener)
    def remove_listener(self, listener): self.listeners.remove(listener)


    def get(self, name, level = None):
        if not name in self.loggers:
            self.loggers[name] = Logger(self, name, self.level)
        return self.loggers[name]


    def _log_time(self, ioloop):
        self._log(datetime.datetime.now().strftime('%Y/%m/%d %H:%M:%S'))
        ioloop.call_later(60 * 60, self._log_time, ioloop)


    def broadcast(self, msg):
        for listener in self.listeners: listener(msg)


    def _log(self, msg, level = INFO, prefix = '', where = None):
        if not msg: return

        hdr = '%s:%s:' % ('DIWE'[level], prefix)
        s = hdr + ('\n' + hdr).join(msg.split('\n'))

        if self.f is not None:
            if 1e22 <= self.bytes_written + len(s) + 1: self._open()
            self.f.write(s + '\n')
            self.f.flush()
            self.bytes_written += len(s) + 1

        print(s)

        # Broadcast to log listeners
        if level == INFO: return

        msg = dict(level = get_level_name(level), source = prefix, msg = msg)
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
            else: self._rotate(path, nextN)

            os.rename(fullpath, '%s.%d' % (path, nextN))
