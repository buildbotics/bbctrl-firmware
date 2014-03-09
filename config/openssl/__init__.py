import os
from SCons.Script import *


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

    conf.CBCheckHome('openssl')

    if env['PLATFORM'] == 'posix': conf.CBCheckLib('dl')

    if (conf.CBCheckCHeader('openssl/ssl.h') and
        (conf.CBCheckLib('crypto') and
         conf.CBCheckLib('ssl')) or
        (conf.CBCheckLib('libeay32') and
         conf.CBCheckLib('ssleay32'))):

        if env['PLATFORM'] == 'win32':
            for lib in ['wsock32', 'advapi32', 'gdi32', 'user32']:
                conf.CBRequireLib(lib)

        if version is not None:
            if not conf.OpenSSLVersion(version):
                raise Exception, 'Insufficient OpenSSL version'

        env.CBDefine('HAVE_OPENSSL')
        return True

    else: raise Exception, 'Need openssl'

    return True


def generate(env):
    env.CBAddConfigTest('openssl', configure)
    env.CBAddTest('OpenSSLVersion', check_version)


def exists():
    return 1

