from SCons.Script import *
from SCons.Action import CommandAction

def build_function(target, source, env):
    top = source[0]
    if 1 < len(source): extra = ' -z ' + ' -z '.join(map(str, source[1:]))
    else: extra = ''

    action = CommandAction('$CXFREEZE $SOURCE --target-dir $TARGET' + extra)
    return action.execute(target, [top], env)


def configure(conf):
    env = conf.env

    env['CXFREEZE'] = 'cxfreeze'
    env['CXFREEZEOPTS'] = ''

    bld = Builder(action = build_function)
    env.Append(BUILDERS = {'cx_Freeze' : bld}) 
