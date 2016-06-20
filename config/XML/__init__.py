import sys
from SCons.Script import *


def configure(conf):
    env = conf.env

    # libexpat
    if conf.CBConfig('expat'): return True

    else: # Glib 2.0
        env.ParseConfig('pkg-config --cflags --libs glib-2.0')
        env.ParseConfig('pkg-config --cflags --libs gthread-2.0')
        if conf.CBCheckHeader('glib.h'):
            env.CBDefine('HAVE_GLIB')
            return True

    return False


def generate(env):
    env.CBAddConfigTest('XML', configure)
    env.CBLoadTool('expat')


def exists():
    return 1
