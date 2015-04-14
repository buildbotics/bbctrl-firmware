import os
from SCons.Script import *


def configure(conf):
    env = conf.env

    conf.CBCheckHome('freetype2', inc_suffix=['/include', '/include/freetype2'])

    if not os.environ.has_key('FREETYPE2_INCLUDE'):
        try:
            env.ParseConfig('freetype-config --cflags')
        except OSError: pass

    if env['PLATFORM'] == 'darwin':
        if not conf.CheckOSXFramework('CoreServices'):
            raise Exception, 'Need CoreServices framework'

    conf.CBRequireCHeader('ft2build.h')
    conf.CBRequireLib('freetype')
    conf.CBConfig('zlib')
    conf.CBCheckLib('png')

    return True


def generate(env):
    env.CBAddConfigTest('freetype2', configure)
    env.CBLoadTools('osx zlib')


def exists():
    return 1
