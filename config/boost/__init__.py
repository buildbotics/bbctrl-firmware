import os
from SCons.Script import *



def try_dirs(paths):
    for parts in paths:
        path = os.path.join(*parts)
        if os.path.isdir(path): return path

    return None


def printenv(key):
    if os.environ.has_key(key):
        print key + '="' + os.environ[key] + '"'


def boost_error(msg):
    printenv('BOOST_SOURCE')
    printenv('BOOST_HOME')
    printenv('BOOST_INCLUDE_PATH')
    printenv('BOOST_LIB_PATH')
    printenv('BOOST_LIB_SUFFIX')
    printenv('BOOST_VERSION')
    raise Exception, msg


def check_version(context, version):
    context.Message("Checking for boost version >= %s..." % version)

    # Boost versions are in format major.minor.subminor
    v_arr = version.split(".")
    version_n = 0
    if len(v_arr) > 0: version_n += int(v_arr[0]) * 100000
    if len(v_arr) > 1: version_n += int(v_arr[1]) * 100
    if len(v_arr) > 2: version_n += int(v_arr[2])

    ret = context.TryRun(
        "#include <boost/version.hpp>\n"
        "int main() {return BOOST_VERSION >= %d ? 0 : 1;}\n" % version_n,
        '.cpp')[0]

    context.Result(ret)
    return ret


def configure(conf, hdrs = [], libs = [], version = '1.35', lib_suffix = ''):
    env = conf.env

    # Find paths
    boost_lib_suffix = lib_suffix
    boost_src = env.get('BOOST_SOURCE')
    boost_ver = env.get('BOOST_VERSION', version)

    if boost_src:
        if not os.path.isdir(boost_src):
            print 'WARNING: BOOST_SOURCE is not a directory'

        if env.get('BOOST_HOME', 0):
            print 'WARNING: Both BOOST_SOURCE & BOOST_HOME are set'

        env.AppendUnique(CPPPATH = [boost_src])

    else: conf.CBCheckHome('boost', inc_suffix =
                           ['/include/boost', '/include/%s/boost' %
                            boost_ver.replace('.', '_'), '/boost'])

    if os.environ.has_key('BOOST_LIB_SUFFIX'):
        boost_lib_suffix = os.environ['BOOST_LIB_SUFFIX'].replace('.', '_')

    # Check version
    if not conf.BoostVersion(boost_ver):
        boost_error('Missing boost version ' + boost_ver)

    # Check headers
    for name in hdrs:
        header = os.path.join('boost', name + '.hpp')
        conf.CBRequireCXXHeader(header)

    # Check libs
    for name in libs:
        libname = 'boost_' + name + boost_lib_suffix
        if env['PLATFORM'] == 'win32': libname = 'lib' + libname
        conf.CBRequireLib(libname)

    # Win32
    if env['PLATFORM'] == 'win32':
        env.CBDefine('BOOST_ALL_NO_LIB')
        env.Prepend(LIBS = ['wsock32'])


def generate(env):
    env.CBAddConfigTest('boost', configure)
    env.CBAddTest('BoostVersion', check_version)

    if 'BOOST_SOURCE' in os.environ:
        env.Replace(BOOST_SOURCE = os.environ.get('BOOST_SOURCE'))
    if 'BOOST_VERSION' in os.environ:
        env.Replace(BOOST_VERSION = os.environ.get('BOOST_VERSION'))


def exists():
    return 1

