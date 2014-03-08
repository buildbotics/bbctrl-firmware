from SCons.Script import *
from SCons.Action import CommandAction


def build_function(target, source, env):
    nsi = str(source[0])

    env.Replace(package = str(target[0]))

    tmp = nsi + '.tmp'
    input = None
    output = None
    try:
        input = open(nsi, 'r')
        output = open(tmp, 'w')
        output.write(input.read() % env)

    finally:
        if input is not None: input.close()
        if output is not None: output.close()

    action = CommandAction('$NSISCOM $NSISOPTS ' + tmp)
    ret = action.execute(target, [tmp], env)
    if ret != 0: return ret

    if 'code_sign_key' in env:
        cmd = '"$SIGNTOOL" sign /f "%s"' % env.get('code_sign_key')
        if 'code_sign_key_pass' in env:
            cmd += ' /p "%s"' % env.get('code_sign_key_pass')
        if 'summary' in env: cmd += ' /d "%s"' % env.get('summary')
        if 'url' in env: cmd += ' /du "%s"' % env.get('url')
        if 'timestamp_url' in env: cmd += ' /t "%s"' % env.get('timestamp_url')
        cmd += ' $TARGET'

        action = CommandAction(cmd)
        return action.execute(target, [], env)

    return 0


def configure(conf):
    env = conf.env

    env['NSISCOM'] = 'makensis'
    env['NSISOPTS'] = ''
    env['SIGNTOOL'] = 'signtool'

    bld = Builder(action = build_function)
    env.Append(BUILDERS = {'Nsis' : bld}) 
