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

import ctypes
import secrets
import subprocess
from tornado.web import HTTPError

from .APIHandler import *
from .Users import MIN_PASSWORD_LEN, ADMIN, generate_passphrase

__all__ = ['AuthHandler']


def call_get_output(cmd):
  p = subprocess.Popen(cmd, stdout = subprocess.PIPE)
  s = p.communicate()[0].decode('utf-8')
  if p.returncode: raise HTTPError(400, 'Command failed')
  return s


def get_username():
  return call_get_output(['getent', 'passwd', '1000']).split(':')[0]


# Hashes a password with crypt(3).  The setting is a salt or an existing hash
# and selects the hash type, e.g. yescrypt, SHA-512 or MD5.
def crypt(password, setting):
  lib = ctypes.CDLL('libcrypt.so.1')
  lib.crypt.restype  = ctypes.c_char_p
  lib.crypt.argtypes = [ctypes.c_char_p, ctypes.c_char_p]

  hash = lib.crypt(password.encode('utf-8'), setting.encode('utf-8'))
  return hash.decode('utf-8') if hash else ''


class AuthHandler(APIHandler):
  auth = False # Logging in must be possible without a session


  def not_found(self): raise HTTPError(404, 'Method not found')
  def  try_call(self, m, *args): getattr(self, m, self.not_found)(*args)
  def       get(self, action, *args): self.try_call('get_'    + action, *args)
  def       put(self, action, *args): self.try_call('put_'    + action, *args)
  def      post(self, action, *args): self.try_call('post_'   + action, *args)
  def    delete(self, action, *args): self.try_call('delete_' + action, *args)


  # Turns user database errors into client errors
  def attempt(self, cb, *args):
    try:
      return cb(*args)
    except HTTPError: raise
    except Exception as e: raise HTTPError(400, str(e))


  def require_name(self, name):
    if not name: raise HTTPError(400, 'User name required')
    return name


  def get_login(self):
    user = self.get_user()

    self.write_json(dict(
      user           = user,
      admin          = self.is_admin(),
      # Admin access which has timed out can be restored by logging in again
      admin_user     = user is not None and self.app.users.is_admin(user),
      admin_left     = round(self.app.sessions.admin_left(self.get_sid())),
      authorized     = self.is_authorized(),
      access_control = self.app.access_control(),
      https          = self.app.https_port))


  def put_login(self):
    self.require_secure() # The password must not be sent in plain text
    password = self.require_arg('password')
    user     = self.json.get('user', ADMIN)

    if user == ADMIN:
      # The built in admin uses the system password of user 1000
      self.not_demo()
      if not self.check_unix_password(password): self.login_failed(user)

    elif not self.app.users.authenticate(user, password):
      self.login_failed(user)

    self.app.sessions.close(self.get_sid()) # Logging in again is allowed
    sid = self.app.sessions.open(user)
    self.set_cookie('bbctrl-sid', sid, httponly = True, samesite = 'Strict',
                    secure = self.request.protocol == 'https')
    self.get_log('Auth').info('User "%s" logged in' % user)

    response = dict(user = user, admin = self.app.sessions.is_admin(sid))

    # An allowed cross origin client cannot use the cookie, it must pass the
    # session ID in an Authorization header
    if self.get_cors_origin(): response['sid'] = sid

    self.write_json(response)


  def delete_login(self):
    self.app.sessions.close(self.get_sid())
    self.clear_cookie('bbctrl-sid')


  # Changing your own password requires proving who is asking, a hijacked
  # session or an unattended browser must not be enough
  def confirm_password(self):
    user = self.get_user()
    current = self.require_arg('current')

    if user == ADMIN:
      self.not_demo()
      ok = self.check_unix_password(current)

    else: ok = self.app.users.authenticate(user, current)

    if not ok:
      self.get_log('Auth').warning(
        'Failed password confirmation for "%s" from %s' %
        (user, self.request.remote_ip))
      raise HTTPError(403, 'Wrong password for "%s"' % user)


  def put_password(self):
    self.require_secure() # The password must not be sent in plain text

    user = self.get_user()
    if user is None: raise HTTPError(401, 'Login required')

    self.confirm_password()
    password = self.require_arg('password')

    if len(password) < MIN_PASSWORD_LEN:
      raise HTTPError(400, 'Password must be at least %d characters' %
                      MIN_PASSWORD_LEN)

    # The built in admin uses the system password of user 1000
    if user == ADMIN:
      self.not_demo()
      self.set_unix_password(password)

    else: self.attempt(self.app.users.set_password, user, password)

    self.get_log('Auth').info('Password changed for "%s"' % user)


  def get_passphrase(self):
    if not self.is_authorized(): raise HTTPError(401, 'Login required')
    self.write_json(dict(passphrase = generate_passphrase()))


  # Written in response to any change so the client cannot be left guessing
  # when enabling access control breaks its connection
  def write_users(self):
    users = self.app.users

    # The built in admin is listed first and cannot be managed here
    entries = [dict(name = ADMIN, admin = True, builtin = True)]
    entries += [dict(name = n, admin = users.is_admin(n), builtin = False)
                for n in users.names()]

    self.write_json(dict(
      access_control = users.access_control(),
      local_access   = users.local_access(),
      users          = entries))


  def get_users(self, name = None):
    self.require_admin()
    self.write_users()


  # Adds a user, fails if the user already exists
  def post_users(self, name = None):
    self.require_admin()
    self.require_secure() # The password must not be sent in plain text

    name     = self.require_name(name)
    password = self.json.get('password', '')
    if not password: raise HTTPError(400, 'Password required')

    self.attempt(self.app.users.add, name, password,
                 self.json.get('admin', None))
    self.get_log('Auth').info(
      'User "%s" added by "%s"' % (name, self.get_user()))
    self.write_users()


  # Changes an existing user's password or admin access
  def put_users(self, name = None):
    self.require_admin()
    self.require_secure() # The password must not be sent in plain text

    name     = self.require_name(name)
    users    = self.app.users
    password = self.json.get('password', '')
    admin    = self.json.get('admin', None)

    if not users.exists(name):
      raise HTTPError(404, 'User "%s" not found' % name)

    if password: self.attempt(users.set_password, name, password)
    if admin is not None: self.attempt(users.set_admin, name, admin)

    self.get_log('Auth').info(
      'User "%s" updated by "%s"' % (name, self.get_user()))
    self.write_users()


  def delete_users(self, name = None):
    self.require_admin()

    name = self.require_name(name)
    self.attempt(self.app.users.remove, name)
    self.get_log('Auth').info(
      'User "%s" removed by "%s"' % (name, self.get_user()))
    self.write_users()


  def put_access(self):
    self.require_admin()

    # A login is useless if passwords are sent in plain text.  Recovery from
    # a lost login is over SSH with the bbctrl-users tool.
    self.require_secure()

    users = self.app.users
    log   = self.get_log('Auth')

    if 'access_control' in self.json:
      enabled = bool(self.json['access_control'])
      self.attempt(users.set_access_control, enabled)
      log.info('Access control %s by "%s"' %
               ('enabled' if enabled else 'disabled', self.get_user()))

    if 'local_access' in self.json:
      enabled = bool(self.json['local_access'])
      self.attempt(users.set_local_access, enabled)
      log.info('Local access %s by "%s"' %
               ('enabled' if enabled else 'disabled', self.get_user()))

    self.write_users()


  def login_failed(self, user):
    self.get_log('Auth').warning(
      'Failed login for "%s" from %s' % (user, self.request.remote_ip))
    raise HTTPError(401, 'Invalid user name or password')


  def check_unix_password(self, password):
    hash = call_get_output(['getent', 'shadow', get_username()]).split(':')[1]

    # Any crypt(3) hash type works.  Disabled logins start with ! or *.
    if not hash.startswith('$'):
      raise HTTPError(401, 'Password login is disabled')

    return secrets.compare_digest(crypt(password, hash), hash)


  def set_unix_password(self, password):
    s = ('%s:%s' % (get_username(), password)).encode('utf-8')
    p = subprocess.Popen(['chpasswd'], stdin = subprocess.PIPE)
    p.communicate(input = s)

    if p.returncode: raise HTTPError(401, 'Failed to set password')
