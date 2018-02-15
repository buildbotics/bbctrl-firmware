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

import re
import logging


log = logging.getLogger('GCode')


class GCodeStream():
    comment1RE = re.compile(r';.*')
    comment2RE = re.compile(r'\(([^\)]*)\)')


    def __init__(self, path):
        self.path = path
        self.f = None

        self.open()


    def close(self):
        if self.f is not None:
            self.f.close()
            self.f = None


    def open(self):
        self.close()

        self.line = 0
        self.f = open('upload' + self.path, 'r')


    def reset(self): self.open()


    def comment(self, s):
        log.debug('Comment: %s', s)


    def next(self):
        line = self.f.readline()
        if line is None or line == '': return

        # Remove comments
        line = self.comment1RE.sub('', line)

        for comment in self.comment2RE.findall(line):
            self.comment(comment)

        line = self.comment2RE.sub(' ', line)

        # Remove space
        line = line.strip()

        # Append line number
        self.line += 1
        line += ' N%d' % self.line

        return line
