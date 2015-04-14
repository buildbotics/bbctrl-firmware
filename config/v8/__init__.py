from SCons.Script import *
import platform


def configure(conf):
    lib_suffix = ['/lib']
    if conf.env.get('debug', False): lib_suffix.append('/build/Release/lib')
    else: lib_suffix.append('/build/Debug/lib')

    conf.CBCheckHome('v8', lib_suffix = lib_suffix)

    if conf.env['PLATFORM'] == 'win32': conf.CBRequireLib('winmm')

    conf.CBRequireCXXHeader('v8.h')

    if not conf.CBCheckLib('v8'):
        conf.CBRequireLib('v8_snapshot')

        if not conf.CBCheckLib('v8_base'):
            if platform.architecture()[0] == '64bit':
                conf.CBRequireLib('v8_base.x64')
            else: conf.CBRequireLib('v8_base.ia32')


def generate(env):
    env.CBAddConfigTest('v8', configure)


def exists():
    return 1

