################################################################################
#                                                                              #
#                 This file is part of the Buildbotics firmware.               #
#                                                                              #
#        Copyright (c) 2015 - 2026, Buildbotics LLC, All rights reserved.      #
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

import time
import secrets

__all__ = ['Sessions']


TIMEOUT = 24 * 60 * 60 # Expire unused sessions


# Login sessions are held in memory only, a restart requires a new login
class Sessions:
    def __init__(self, users, timeout = TIMEOUT):
        self.users    = users
        self.timeout  = timeout
        self.sessions = {}


    def open(self, user):
        sid = secrets.token_urlsafe(16)
        self.sessions[sid] = [user, time.time()]
        return sid


    def close(self, sid): self.sessions.pop(sid, None)


    def get_user(self, sid):
        # Returns the session's user name or None if the session is invalid
        self._expire()

        session = self.sessions.get(sid)
        if session is None: return

        # Users removed via the command line lose their sessions
        if not self.users.exists(session[0]):
            self.close(sid)
            return

        session[1] = time.time() # Touch
        return session[0]


    def is_admin(self, sid):
        user = self.get_user(sid)
        return user is not None and self.users.is_admin(user)


    def _expire(self):
        cutoff = time.time() - self.timeout
        for sid in [s for s, v in self.sessions.items() if v[1] < cutoff]:
            del self.sessions[sid]
