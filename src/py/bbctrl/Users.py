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

import os
import re
import json
import hashlib
import secrets

__all__ = ['Users', 'USERS_PATH', 'MIN_PASSWORD_LEN', 'ADMIN',
           'generate_passphrase']


# The built in administrator.  It always exists, is never listed with the
# other users and is authenticated against the system password of user 1000.
ADMIN            = 'admin'
USERS_PATH       = '/etc/bbctrl/users.json'
MIN_PASSWORD_LEN = 6
SCRYPT           = dict(n = 1 << 14, r = 8, p = 1)
NAME_RE          = re.compile(r'^[A-Za-z0-9][A-Za-z0-9._-]{0,31}$')
PASSPHRASE_WORDS = 4
WORDLIST         = 'bip39.txt'


# Generates a passphrase from the BIP-39 English word list.  With the default
# of 4 words out of 2048 this is 44 bits of entropy.  NOTE, the word list is
# loaded relative to this file so that the command line tool keeps working
# even if the package metadata is broken.
def generate_passphrase(words = PASSPHRASE_WORDS):
    path = os.path.join(os.path.dirname(os.path.abspath(__file__)), WORDLIST)

    with open(path, 'r') as f: wordlist = f.read().split()

    return ' '.join(secrets.choice(wordlist) for i in range(words))


def _scrypt(password, salt, n, r, p):
    key = hashlib.scrypt(password.encode('utf-8'), salt = bytes.fromhex(salt),
                         n = n, r = r, p = p, dklen = 32)
    return 'scrypt$%d$%d$%d$%s$%s' % (n, r, p, salt, key.hex())


def hash_password(password):
    if len(password) < MIN_PASSWORD_LEN:
        raise Exception('Password must be at least %d characters' %
                        MIN_PASSWORD_LEN)

    return _scrypt(password, secrets.token_hex(16), **SCRYPT)


def check_password(password, hash):
    try:
        type, n, r, p, salt, digest = hash.split('$')
        if type != 'scrypt': return False
        computed = _scrypt(password, salt, int(n), int(r), int(p))

    except (AttributeError, ValueError): return False

    return secrets.compare_digest(computed, hash)


# Web interface users.  These are not related to the Unix system users.
# The database is only read or written by root and is reloaded automatically
# when changed by the command line tool.
class Users:
    def __init__(self, path = USERS_PATH, log = None):
        self.path    = path
        self.log     = log
        self.version = None
        self.error   = False
        self.data    = {}

        self.reload()


    def _version(self):
        # The inode changes on every save because it is written atomically
        s = os.stat(self.path)
        return (s.st_mtime_ns, s.st_ino, s.st_size)


    def reload(self):
        try:
            version = self._version()

        except OSError:
            self.data, self.version, self.error = {}, None, False
            return

        if version == self.version: return

        try:
            with open(self.path, 'r') as f: data = json.load(f)
            if not isinstance(data.get('users', {}), dict):
                raise Exception('Invalid "users" entry')

            self.data, self.version, self.error = data, version, False

        except Exception as e:
            # Fail closed, an unreadable database must not open up access
            self.version, self.error = version, True
            if self.log is not None:
                self.log.error('Failed to load %s: %s' % (self.path, e))


    def users(self):
        self.reload()
        return self.data.setdefault('users', {})


    def access_control(self):
        # Enabled by default once users have been created
        users = self.users()
        if self.error: return True # Fail closed
        return self.data.get('access-control', bool(users))


    def local_access(self):
        self.reload()
        return self.data.get('local-access', True)


    def names(self): return sorted(self.users().keys())


    def exists(self, name):
        return name == ADMIN or name in self.users()


    def is_admin(self, name):
        return name == ADMIN or bool(self._get(name).get('admin'))


    def authenticate(self, name, password):
        if name == ADMIN:
            raise Exception('"%s" uses the system password' % ADMIN)

        user = self.users().get(name)
        if user is None: return False
        return check_password(password, user.get('hash', ''))


    def add(self, name, password, admin = None):
        users = self.users()

        if name == ADMIN:
            raise Exception('"%s" is reserved for the built in administrator' %
                            ADMIN)

        if not NAME_RE.match(name):
            raise Exception('Invalid user name "%s"' % name)

        if name in users: raise Exception('User "%s" already exists' % name)

        # The first user is always an admin
        if admin is None: admin = not users

        users[name] = dict(admin = bool(admin), hash = hash_password(password))
        self.save()


    def remove(self, name):
        self._require(name)

        # Removing users must not change whether a login is required.  The
        # built in admin can always log in.
        self.data.setdefault('access-control', self.access_control())

        del self.users()[name]
        self.save()


    def set_password(self, name, password):
        self._require(name)['hash'] = hash_password(password)
        self.save()


    def set_admin(self, name, admin):
        self._require(name)['admin'] = bool(admin)
        self.save()


    def set_access_control(self, enable):
        self.reload()
        self.data['access-control'] = bool(enable)
        self.save()


    def set_local_access(self, enable):
        self.reload()
        self.data['local-access'] = bool(enable)
        self.save()


    def save(self):
        dir = os.path.dirname(self.path)
        if dir and not os.path.isdir(dir): os.makedirs(dir, mode = 0o755)

        # Create with restricted permissions and replace atomically
        tmp   = self.path + '.tmp'
        flags = os.O_WRONLY | os.O_CREAT | os.O_TRUNC

        with open(os.open(tmp, flags, 0o600), 'w') as f:
            json.dump(self.data, f, indent = 2, separators = (',', ': '))
            f.flush()
            os.fsync(f.fileno())

        os.rename(tmp, self.path)
        os.sync()

        self.version, self.error = self._version(), False


    def _get(self, name): return self.users().get(name, {})


    def _require(self, name):
        if name == ADMIN:
            raise Exception('"%s" is the built in administrator and cannot be '
                            'changed here' % ADMIN)

        user = self.users().get(name)
        if user is None: raise Exception('User "%s" not found' % name)
        return user
