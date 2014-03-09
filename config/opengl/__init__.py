from SCons.Script import *
import config


def configure(conf):
    env = conf.env

    conf.CBCheckHome('GL')

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

        conf.CBRequireLib(gllib)
        conf.CBRequireLib(glulib)


def generate(env):
    env.CBAddConfigTest('opengl', configure)
    env.CBLoadTool('osx')


def exists():
    return 1
