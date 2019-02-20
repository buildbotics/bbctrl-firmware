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

import json
import traceback
import bbctrl

from tornado.web import HTTPError
import tornado.httpclient


class APIHandler(bbctrl.RequestHandler):
    def delete(self, *args, **kwargs):
        self.delete_ok(*args, **kwargs)
        self.write_json('ok')


    def delete_ok(self): raise HTTPError(405)


    def put(self, *args, **kwargs):
        self.put_ok(*args, **kwargs)
        self.write_json('ok')


    def put_ok(self): raise HTTPError(405)


    def prepare(self):
        self.json = {}

        if self.request.body:
            try:
                self.json = tornado.escape.json_decode(self.request.body)
            except ValueError:
                raise HTTPError(400, 'Unable to parse JSON')


    def set_default_headers(self):
        self.set_header('Content-Type', 'application/json')


    def write_error(self, status_code, **kwargs):
        e = {}

        if 'message' in kwargs: e['message'] = kwargs['message']

        elif 'exc_info' in kwargs:
            typ, value, tb = kwargs['exc_info']
            if isinstance(value, HTTPError) and value.log_message:
                e['message'] = value.log_message % value.args
            else: e['message'] = str(kwargs['exc_info'][1])

        else: e['message'] = 'Unknown error'

        e['code'] = status_code

        self.write_json(e)


    def write_json(self, data, pretty = False):
        if pretty: data = json.dumps(data, indent = 2, separators = (',', ': '))
        else: data = json.dumps(data, separators = (',', ':'))
        self.write(data)
