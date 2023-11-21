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

import json
import traceback
from tornado.web import HTTPError
import tornado.httpclient

from .RequestHandler import *

__all__ = ['APIHandler']


class APIHandler(RequestHandler):
    def require_arg(self, name):
        if not name in self.json:
            raise HTTPError(400, 'Argument "%s" required' % name)

        return self.json[name]


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
