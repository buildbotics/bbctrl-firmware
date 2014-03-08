from SCons.Script import *
import config


deps = ['osx']


def configure(conf):
    env = conf.env

    config.check_home(conf, 'GL')

    if env['PLATFORM'] == 'darwin':
        config.configure('osx', conf)

        if not (conf.CheckOSXFramework('OpenGL') and
                conf.CheckCHeader('OpenGL/gl.h')):
            raise Exception, 'Need OpenGL'

    else:
        if env['PLATFORM'] == 'win32':
            glulib = 'glu32'
            gllib = 'opengl32'

        else:
            glulib = 'GLU'
            gllib = 'GL'

        config.require_lib(conf, gllib)
        config.require_lib(conf, glulib)

    return True
