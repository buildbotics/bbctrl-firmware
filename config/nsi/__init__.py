from SCons.Script import *
from SCons.Action import CommandAction


def build_function(target, source, env):
    nsi = str(source[0])

    env.Replace(package = str(target[0]))

    if not 'PACKAGE_ARCH' in env:
        import platform
        env['PACKAGE_ARCH'] = platform.architecture()[0][:2]

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

    if env.get('code_sign_key', None):
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


def generate(env):
    env.SetDefault(NSISCOM = 'makensis', NSISOPTS = '', SIGNTOOL = 'signtool')
    bld = Builder(action = build_function)
    env.Append(BUILDERS = {'Nsis' : bld}) 


def exists():
    return 1

