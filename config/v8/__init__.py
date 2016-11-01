from SCons.Script import *
import platform
import os


version_src = """
#include <v8-version.h>

int main() {
  return
    (%(major)s < V8_MAJOR_VERSION || (%(major)s == V8_MAJOR_VERSION &&
    (%(minor)s < V8_MINOR_VERSION || (%(minor)s == V8_MINOR_VERSION &&
     %(build)s <= V8_BUILD_NUMBER)))) ? 0 : 1;
}
"""


def check_version(context, version):
    context.Message("Checking for v8 version >= %s..." % version)

    major, minor, build = version.split('.')
    version = {'major': major, 'minor': minor, 'build': build}

    ret = context.TryRun(version_src % version, '.cpp')[0]

    context.Result(ret)

    return ret


def configure(conf):
    lib_suffix = ['/lib']
    if conf.env.get('debug', False): lib_suffix.append('/build/Release/lib')
    else: lib_suffix.append('/build/Debug/lib')

    conf.CBCheckHome('v8', lib_suffix = lib_suffix)

    version = conf.env.get('V8_VERSION')
    if not conf.V8Version(version): return False

    if conf.env['PLATFORM'] == 'win32' or int(conf.env.get('cross_mingw', 0)):
        conf.CBRequireLib('winmm')

    conf.CBRequireCXXHeader('v8.h')
    conf.CBRequireCXXHeader('v8-version.h')

    if not conf.CBCheckLib('v8'):
        conf.CBRequireLib('v8_snapshot')

        if not conf.CBCheckLib('v8_base'):
            if platform.architecture()[0] == '64bit':
                conf.CBRequireLib('v8_base.x64')
            else: conf.CBRequireLib('v8_base.ia32')


def generate(env):
    env.CBAddConfigTest('v8', configure)
    env.CBAddTest('V8Version', check_version)
    env.Replace(V8_VERSION = os.environ.get('V8_VERSION', '5.6.149'))


def exists():
    return 1
