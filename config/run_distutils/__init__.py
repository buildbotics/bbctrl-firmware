from SCons.Script import *


def generate(env):
    env.SetDefault(RUN_DISTUTILS = 'python')
    env.SetDefault(RUN_DISTUTILSOPTS = 'build')

    env['RUN_DISTUTILS'] = 'python'
    env['RUN_DISTUTILSOPTS'] = 'build'

    bld = Builder(action = '$RUN_DISTUTILS $SOURCE $RUN_DISTUTILSOPTS')
    env.Append(BUILDERS = {'RunDistUtils' : bld}) 


def exists():
    return 1
