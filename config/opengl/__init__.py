from SCons.Script import *


def configure(conf):
    env = conf.env

    conf.CBCheckHome('GL')

    if env['PLATFORM'] == 'darwin' or int(env.get('cross_osx', 0)):
        if not (conf.CheckOSXFramework('OpenGL') and
                conf.CheckCHeader('OpenGL/gl.h')):
            raise Exception, 'Need OpenGL'

    else:
        if env['PLATFORM'] == 'win32' or int(env.get('cross_mingw', 0)):
            glulib = 'glu32'
            gllib = 'opengl32'

        else:
            glulib = 'GLU'
            gllib = 'GL'

        conf.CBRequireLib(gllib)
        conf.CBRequireLib(glulib)


def generate(env):
    env.CBAddConfigTest('opengl', configure)
    env.CBLoadTool('osx')


def exists():
    return 1
