from SCons.Script import *

def configure(conf):
    env = conf.env

    env['RUN_DISTUTILS'] = 'python'
    env['RUN_DISTUTILSOPTS'] = 'build'

    bld = Builder(action = '$RUN_DISTUTILS $SOURCE $RUN_DISTUTILSOPTS')
    env.Append(BUILDERS = {'RunDistUtils' : bld}) 
