################################################################################
#                                                                              #
#                 This file is part of the Buildbotics firmware.               #
#                                                                              #
#        Copyright (c) 2015 - 2023, Buildbotics LLC, All rights reserved.      #
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
from tornado.web import HTTPError
import tornado.web

from .Log import *

__all__ = ['RequestHandler', 'get_bearer_sid']

BEARER = 'Bearer '


# Cross origin clients cannot use the session cookie so they pass the session
# ID in an Authorization header instead
def get_bearer_sid(request):
    auth = request.headers.get('Authorization', '')
    if auth.startswith(BEARER): return auth[len(BEARER):].strip()


class RequestHandler(tornado.web.RequestHandler):
    auth = True # Set False to allow access without a login


    def __init__(self, app, request, **kwargs):
        super().__init__(app, request, **kwargs)
        self.app = app
        self.set_cors_headers()


    def prepare(self):
        # CORS preflight requests carry no cookies and no data.  They must be
        # answered where they are sent because browsers will not follow a
        # redirect for a preflight.
        if self.request.method == 'OPTIONS': return

        if self.redirect_https(): return
        if self.auth: self.require_session()


    def get_ctrl(self):
        return self.app.get_ctrl(self.get_cookie('bbctrl-client-id'))


    def get_log(self, name = 'API'): return self.get_ctrl().log.get(name)
    def get_events(self): return self.get_ctrl().events


    def get_sid(self):
        return get_bearer_sid(self.request) or self.get_cookie('bbctrl-sid')
    def get_user(self): return self.app.sessions.get_user(self.get_sid())
    def is_admin(self): return self.app.sessions.is_admin(self.get_sid())


    def is_authorized(self):
        return self.app.is_authorized(self.get_sid(), self.request.remote_ip)


    def redirect_https(self):
        if not self.app.needs_https(self.request.protocol,
                                    self.request.remote_ip): return False

        port = self.app.https_port
        host = self.request.host_name
        if port != 443: host += ':%d' % port

        self.set_status(307) # Preserve the request method
        self.set_header('Location', 'https://%s%s' % (host, self.request.uri))
        self.finish()

        return True


    def require_session(self):
        if not self.is_authorized(): raise HTTPError(401, 'Login required')


    def require_admin(self):
        if self.get_user() is None: raise HTTPError(401, 'Login required')
        if not self.is_admin(): raise HTTPError(403, 'Admin access required')


    def not_demo(self):
        if self.get_ctrl().args.demo:
            raise HTTPError(400, 'Not supported in demo mode')


    def emit(self, event, *args, **kwargs):
        self.get_events().emit(event, *args, **kwargs)


    # Override exception logging
    def log_exception(self, typ, value, tb):
        if (isinstance(value, HTTPError) and
            400 <= value.status_code and value.status_code < 500): return

        log = self.get_log()
        log.set_level(Log.DEBUG)

        log.error(str(value))
        trace = ''.join(traceback.format_exception(typ, value, tb))
        log.debug(trace)


    def options(self, *args):
        self.set_status(204)
        self.finish()


    def get_cors_origin(self):
        # Returns the request's origin if it is allowed
        origin = self.request.headers.get('Origin', '')
        origins = self.get_ctrl().config.get('cors-origins', '').split()
        if origin and origin in origins: return origin


    def set_cors_headers(self):
        origin = self.get_cors_origin()
        if origin:
            self.set_header('Access-Control-Allow-Origin', origin)
            self.set_header(
                'Access-Control-Allow-Headers', 'DNT,User-Agent,' +
                'X-Requested-With,If-Modified-Since,Cache-Control,' +
                'Content-Type,Range,Set-Cookie,Authorization')
            self.set_header(
                'Access-Control-Allow-Methods',
                'POST,PUT,GET,OPTIONS,DELETE')
