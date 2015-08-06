from SCons.Script import *


def configure(conf):
    env = conf.env

    conf.CBCheckHome('re2')

    conf.CBRequireCXXHeader('re2.h')
    conf.CBRequireLib('re2')


def generate(env):
    env.CBAddConfigTest('re2', configure)


def exists():
    return 1
