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


TIMEOUT       = 24 * 60 * 60 # Expire unused sessions
ADMIN_TIMEOUT = 5 * 60       # Expire unused admin access


# Login sessions are held in memory only, a restart requires a new login
class Sessions:
    def __init__(self, users, timeout = TIMEOUT,
                 admin_timeout = ADMIN_TIMEOUT):
        self.users         = users
        self.timeout       = timeout
        self.admin_timeout = admin_timeout
        self.sessions      = {}


    def open(self, user):
        sid = secrets.token_urlsafe(16)
        now = time.time()
        self.sessions[sid] = [user, now, now]
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


    def admin_left(self, sid):
        # Returns the seconds of admin access left, zero if there is none
        user = self.get_user(sid)
        if user is None or not self.users.is_admin(user): return 0

        left = self.admin_timeout - (time.time() - self.sessions[sid][2])
        return max(0, left)


    # Admin access expires so that it cannot be left standing on an
    # unattended browser.  Using it renews it, like sudo.
    def is_admin(self, sid, renew = False):
        if not self.admin_left(sid): return False
        if renew: self.sessions[sid][2] = time.time()
        return True


    def _expire(self):
        cutoff = time.time() - self.timeout
        for sid in [s for s, v in self.sessions.items() if v[1] < cutoff]:
            del self.sessions[sid]
