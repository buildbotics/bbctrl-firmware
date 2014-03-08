import os
from SCons.Script import *
import config



def try_dirs(paths):
    for parts in paths:
        path = ''
        for part in parts:
            path = os.path.join(path, part)
        if os.path.isdir(path): return path

    return None


def printenv(key):
    if os.environ.has_key(key):
        print key + '="' + os.environ[key] + '"'


def boost_error(msg):
    print msg
    printenv('BOOST_SOURCE')
    printenv('BOOST_HOME')
    printenv('BOOST_INCLUDE_PATH')
    printenv('BOOST_LIB_PATH')
    printenv('BOOST_LIB_SUFFIX')
    printenv('BOOST_VERSION')
    Exit(1)


def check_version(context, version):
    context.Message("Checking for boost version >= %s..." % version)

    # Boost versions are in format major.minor.subminor
    v_arr = version.split(".")
    version_n = 0
    if len(v_arr) > 0: version_n += int(v_arr[0]) * 100000
    if len(v_arr) > 1: version_n += int(v_arr[1]) * 100
    if len(v_arr) > 2: version_n += int(v_arr[2])

    ret = context.TryRun("""#include <boost/version.hpp>
     int main() {return BOOST_VERSION >= %d ? 0 : 1;}
     \n""" % version_n, '.cpp')[0]

    context.Result(ret)
    return ret


def configure(conf, hdrs = [], libs = [], version = '1.35', lib_suffix = ''):
    env = conf.env

    # Find paths
    boost_inc = None
    boost_lib = None
    boost_lib_suffix = lib_suffix

    boost_inc = os.environ.get('BOOST_SOURCE', boost_inc)
    boost_ver = os.environ.get('BOOST_VERSION', version)

    boost_home = config.check_home(conf, 'boost')
    if boost_home:
        path = try_dirs([[boost_home, 'include', 'boost'],
                         [boost_home, 'include',
                          'boost-' + boost_ver.replace('.', '_'), 'boost'],
                         [boost_home, 'boost']])
        if not boost_inc:
            if path: boost_inc = os.path.dirname(path)
            else: print "WARNING: No boost include path found in BOOST_HOME"

        path = os.path.join(boost_home, 'lib')
        if os.path.isdir(path): boost_lib = path
        else: print "WARNING: No boost lib path found in BOOST_HOME"

    if boost_inc: env.AppendUnique(CPPPATH = [boost_inc])
    if boost_lib: env.AppendUnique(LIBPATH = [boost_lib])

    if os.environ.has_key('BOOST_LIB_SUFFIX'):
        boost_lib_suffix = os.environ['BOOST_LIB_SUFFIX'].replace('.', '_')

    # Check version
    conf.AddTest('BoostVersion', check_version)
    if not conf.BoostVersion(boost_ver):
        boost_error("Wrong version")

    # Check headers
    for name in hdrs:
        header = os.path.join('boost', name + '.hpp')
        config.require_cxx_header(conf, header)

    # Check libs
    for name in libs:
        libname = 'boost_' + name + boost_lib_suffix
        if env['PLATFORM'] == 'win32': libname = 'lib' + libname
        config.require_lib(conf, libname)

    # Win32
    if env['PLATFORM'] == 'win32':
        env.AppendUnique(CPPDEFINES = ['BOOST_ALL_NO_LIB'])
        env.Prepend(LIBS = ['wsock32'])
