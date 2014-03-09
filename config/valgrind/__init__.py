from SCons.Script import *


def configure(conf):
    env = conf.env

    if conf.CBCheckHeader('valgrind/valgrind.h'):
        env.CBDefine('HAVE_VALGRIND_H')

    if conf.CBCheckHeader('valgrind/drd.h'):
        env.CBDefine('HAVE_VALGRIND_DRD_H')

    return True


def generate(env):
    env.CBAddConfigTest('valgrind', configure)


def exists():
    return 1

