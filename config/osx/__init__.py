import platform
from SCons.Script import *


def CheckOSXFramework(ctx, name):
    if (platform.system().lower() == 'darwin'):
        ctx.Message('Checking for framework %s... ' % name)
        save_FRAMEWORKS = ctx.env['FRAMEWORKS']
        ctx.env.PrependUnique(FRAMEWORKS = [name])
        result = \
            ctx.TryLink('int main(int argc, char **argv) {return 0;}', '.c')
        ctx.Result(result)

        if not result:
            ctx.env.Replace(FRAMEWORKS = save_FRAMEWORKS)

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

