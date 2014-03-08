import os
from SCons.Script import *
import config


def check_version(context, version):
    context.Message("Checking for openssl version >= %s..." % version)

    version = version.split('.')
    src = '#include <openssl/opensslv.h>\nint main() {\nreturn ('
    src += version[0] + '<= (OPENSSL_VERSION_NUMBER >> 28)'
    if 1 < len(version):
        src += ' && ' + version[1] + \
               '<= ((OPENSSL_VERSION_NUMBER >> 20) & 0xf)'
    if 2 < len(version):
        src += ' && ' + version[2] + \
               '<= ((OPENSSL_VERSION_NUMBER >> 12) & 0xf)'
    src += ') ? 0 : 1;}\n'

    ret = context.TryRun(src, '.cpp')[0]

    context.Result(ret)
    return ret


def configure(conf, version = None):
    env = conf.env

    if os.environ.has_key('OPENSSL_HOME'):
        home = os.environ['OPENSSL_HOME']
        if os.path.exists(home + '/inc32') and os.path.exists(home + '/out32'):
            env.AppendUnique(CPPPATH = [home + '/inc32'])
            env.AppendUnique(LIBPATH = [home + '/out32'])
        else:
            if os.path.exists(home + '/include'):
                env.AppendUnique(CPPPATH = [home + '/include'])
            if os.path.exists(home + '/lib'):
                env.AppendUnique(LIBPATH = [home + '/lib'])
            else: env.AppendUnique(LIBPATH = [home])

    config.check_home(conf, 'openssl')

    if env['PLATFORM'] == 'posix': config.check_lib(conf, 'dl')

    if (conf.CheckCHeader('openssl/ssl.h') and
        (config.check_lib(conf, 'crypto') and
         config.check_lib(conf, 'ssl')) or
        (config.check_lib(conf, 'libeay32') and
         config.check_lib(conf, 'ssleay32'))):

        if env['PLATFORM'] == 'win32':
            for lib in ['wsock32', 'advapi32', 'gdi32', 'user32']:
                config.require_lib(conf, lib)

        if version is not None:
            conf.AddTest('OpenSSLVersion', check_version)
            if not conf.OpenSSLVersion(version):
                raise Exception, 'Insufficient OpenSSL version'

        env.AppendUnique(CPPDEFINES = ['HAVE_OPENSSL'])
        return True

    else: raise Exception, 'Need openssl'

