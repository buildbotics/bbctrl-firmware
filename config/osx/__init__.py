import platform
from SCons.Script import *


def CheckOSXFramework(ctx, name):
    env = ctx.env

    if platform.system().lower() == 'darwin' or int(env.get('cross_osx', 0)):
        ctx.Message('Checking for framework %s... ' % name)
        save_FRAMEWORKS = env['FRAMEWORKS']
        env.PrependUnique(FRAMEWORKS = [name])
        result = \
            ctx.TryLink('int main(int argc, char **argv) {return 0;}', '.c')
        ctx.Result(result)

        if not result:
            env.Replace(FRAMEWORKS = save_FRAMEWORKS)

        return result

    ctx.Result(False)
    return False


def RequireOSXFramework(ctx, name):
    if not ctx.sconf.CheckOSXFramework(name):
        raise Exception, 'Need Framework ' + name


def generate(env):
    env.CBAddTest(CheckOSXFramework)
    env.CBAddTest(RequireOSXFramework)


def exists():
    return 1
