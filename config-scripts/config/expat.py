from SCons.Script import *
import config


def configure(conf):
    env = conf.env

    config.check_home(conf, 'expat')

    libname = 'expat'
    if env['PLATFORM'] == 'win32':
        libname = 'lib' + libname + 'MT'
        env.AppendUnique(CPPDEFINES = ['XML_STATIC'])

    config.require_header(conf, 'expat.h')
    config.require_lib(conf, libname)
    env.AppendUnique(CPPDEFINES = ['HAVE_EXPAT'])
