from SCons.Script import *


def configure(conf):
    conf.CBCheckHome('bzip2', lib_suffix = ['', '/lib'],
                     inc_suffix = ['/src', '/include'])
    conf.CBRequireHeader('bzlib.h')
    conf.CBRequireLib('bz2')


def generate(env):
    env.CBAddConfigTest('bzip2', configure)


def exists():
    return 1

