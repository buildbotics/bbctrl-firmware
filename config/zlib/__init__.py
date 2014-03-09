from SCons.Script import *


def configure(conf):
    conf.CBCheckHome('zlib', lib_suffix = ['', '/lib'],
                     inc_suffix = ['/src', '/include'])
    conf.CBRequireHeader('zlib.h')
    conf.CBRequireLib('z')


def generate(env):
    env.CBAddConfigTest('zlib', configure)


def exists():
    return 1

