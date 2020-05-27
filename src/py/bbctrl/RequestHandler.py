################################################################################
#                                                                              #
#                 This file is part of the Buildbotics firmware.               #
#                                                                              #
#        Copyright (c) 2015 - 2020, Buildbotics LLC, All rights reserved.      #
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
            400 <= value.status_code and value.status_code < 500): return

        log = self.get_log()
        log.set_level(bbctrl.log.DEBUG)

        log.error(str(value))
        trace = ''.join(traceback.format_exception(typ, value, tb))
        log.debug(trace)
