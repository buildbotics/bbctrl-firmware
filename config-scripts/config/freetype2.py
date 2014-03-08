import os
from SCons.Script import *
import config

deps = ['osx']

def configure(conf):
    env = conf.env

    config.check_home(conf, 'freetype2')

    if not os.environ.has_key('FREETYPE2_INCLUDE'):
        try:
            env.ParseConfig('freetype-config --cflags')
        except OSError: pass

    if env['PLATFORM'] == 'darwin':
        config.configure('osx', conf)
        if not conf.CheckOSXFramework('CoreServices'):
            raise Exception, 'Need CoreServices framework'

    config.require_header(conf, 'ft2build.h')
    config.require_lib(conf, 'freetype')

    config.check_home(conf, 'zlib', lib_suffix = '') # TODO Hack!
    config.require_lib(conf, 'z')
    config.check_lib(conf, 'png')

    return True
