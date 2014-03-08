from SCons.Script import *
import config


def configure(conf):
    env = conf.env

    config.check_home(conf, 'GLEW')

    if env['PLATFORM'] == 'win32':
        glewlib = 'glew32s' # The static version
        env.AppendUnique(CPPDEFINES = ['GLEW_BUILD=GLEW_STATIC'])

    else: glewlib = 'GLEW'

    config.require_header(conf, 'GL/glew.h')
    config.require_lib(conf, glewlib)

    return True
