from SCons.Script import *


def configure(conf):
    conf.CBCheckHome('duktape', lib_suffix = [''], inc_suffix = [''])
    conf.CBRequireHeader('duktape.h')
    conf.CBRequireLib('duktape')


def generate(env):
    env.CBAddConfigTest('duktape', configure)


def exists():
    return 1
