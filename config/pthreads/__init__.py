from SCons.Script import *


def configure(conf):
    conf.CBCheckHome('pthreads')
    conf.CBRequireHeader('pthread.h')
    conf.CBRequireLib('pthread')
    conf.env.CBDefine('HAVE_PTHREADS')


def generate(env):
    env.CBAddConfigTest('pthreads', configure)
    env.CBAddTest('ConfigPThreads', configure)


def exists():
    return 1

