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
import logging
import bbctrl


log = logging.getLogger('Msgs')


class Messages(logging.Handler):
    def __init__(self, ctrl):
        logging.Handler.__init__(self, logging.WARNING)

        self.ctrl = ctrl
        self.listeners = []

        logging.getLogger().addHandler(self)


    def add_listener(self, listener): self.listeners.append(listener)
    def remove_listener(self, listener): self.listeners.remove(listener)


    # From logging.Handler
    def emit(self, record):
        msg = dict(
            level = record.levelname.lower(),
            source = record.name,
            msg = record.getMessage())

        if hasattr(record, 'where'): msg['where'] = record.where
        else: msg['where'] = '%s:%d' % (record.filename, record.lineno)

        for listener in self.listeners:
            listener(msg)
