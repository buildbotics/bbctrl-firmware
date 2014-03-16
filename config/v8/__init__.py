from SCons.Script import *


def configure(conf):
    conf.CBCheckHome('v8')

    if conf.env['PLATFORM'] == 'win32':
        if not conf.CBCheckLib('winmm'): return False

    if not conf.CBCheckCXXHeader('v8.h'): return False

    if conf.CBCheckLib('v8'): return True
    if not conf.CBCheckLib('v8_snapshot'): return False

    return conf.CBCheckLib('v8_base') or conf.CBCheckLib('v8_base.ia32')


def generate(env):
    env.CBAddConfigTest('v8', configure)


def exists():
    return 1

