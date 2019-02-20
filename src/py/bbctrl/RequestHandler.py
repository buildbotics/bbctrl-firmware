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

import traceback
import bbctrl

from tornado.web import HTTPError
import tornado.web


class RequestHandler(tornado.web.RequestHandler):
    def __init__(self, app, request, **kwargs):
        super().__init__(app, request, **kwargs)
        self.app = app


    def get_ctrl(self): return self.app.get_ctrl(self.get_cookie('client-id'))
    def get_log(self, name = 'API'): return self.get_ctrl().log.get(name)


    def get_path(self, path = None, filename = None):
        return self.get_ctrl().get_path(path, filename)


    def get_upload(self, filename = None):
        return self.get_ctrl().get_upload(filename)


    # Override exception logging
    def log_exception(self, typ, value, tb):
        if (isinstance(value, HTTPError) and
            value.status_code in (400, 404, 408)): return

        log = self.get_log()
        log.set_level(bbctrl.log.DEBUG)

        log.error(str(value))
        trace = ''.join(traceback.format_exception(typ, value, tb))
        log.debug(trace)
