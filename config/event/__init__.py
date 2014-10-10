from SCons.Script import *


def configure(conf):
    conf.CBCheckHome('event', lib_suffix = ['/src'], inc_suffix = ['/include'])
    conf.CBRequireHeader('event.h')
    conf.CBRequireLib('event')


def generate(env):
    env.CBAddConfigTest('event', configure)


def exists():
    return 1

