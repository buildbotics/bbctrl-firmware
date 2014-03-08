from SCons.Script import *
import config


deps = ['osx']


def configure(conf):
    env = conf.env

    config.check_home(conf, 'GLUT')

    if env['PLATFORM'] == 'darwin':
        config.configure('osx', conf)

        if not (conf.CheckOSXFramework('GLUT') and
                conf.CheckCHeader('GLUT/glut.h')):
            raise Exception, 'Need GLUT'

    else:
        if env['PLATFORM'] == 'win32':
            freeglutlib = 'freeglut_static'
            glutlib = 'glut32'

            env.AppendUnique(CPPDEFINES = ['FREEGLUT_STATIC'])

        else: freeglutlib = glutlib = 'glut'

        config.require_header(conf, 'GL/glut.h')

        if not (config.check_lib(conf, freeglutlib) or
                config.check_lib(conf, glutlib)):
            raise Exception, 'Need glut'

    return True
