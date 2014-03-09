from SCons.Script import *
import config


def configure(conf):
    env = conf.env

    conf.CBCheckHome('expat')

    libname = 'expat'
    if env['PLATFORM'] == 'win32':
        libname = 'lib' + libname + 'MT'
        env.CBDefine('XML_STATIC')

    conf.CBRequireCHeader('expat.h')
    conf.CBRequireLib(libname)
    env.CBDefine('HAVE_EXPAT')


def generate(env):
    env.CBAddConfigTest('expat', configure)


def exists():
    return 1
