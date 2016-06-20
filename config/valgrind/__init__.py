from SCons.Script import *


def configure(conf):
    env = conf.env

    if conf.CBCheckHeader('valgrind/valgrind.h') and \
            conf.CBCheckHeader('valgrind/drd.h') and \
            conf.CBCheckHeader('valgrind/helgrind.h') and \
            conf.CBCheckHeader('valgrind/memcheck.h'):
        env.CBDefine('HAVE_VALGRIND')


def generate(env):
    env.CBAddConfigTest('valgrind', configure)


def exists():
    return 1
