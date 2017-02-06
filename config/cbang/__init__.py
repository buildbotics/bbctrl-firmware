from SCons.Script import *
import inspect
import os


def GetHome():
    path = inspect.getfile(inspect.currentframe())
    return os.path.dirname(os.path.abspath(path))


def configure_deps(conf, local = True, with_openssl = True):
    env = conf.env

    conf.CBConfig('zlib', not local)
    conf.CBConfig('bzip2', not local)
    conf.CBConfig('XML', not local)
    conf.CBConfig('sqlite3', not local)
    if conf.CBConfig('event', False): conf.CBConfig('re2', not local)

    if conf.CBCheckLib('leveldb') and conf.CBCheckLib('snappy'):
        conf.CBCheckHome('leveldb')
        if conf.CBCheckCXXHeader('leveldb/db.h'):
            env.CBDefine('HAVE_LEVELDB')

    if conf.CBCheckCHeader('mysql/mysql.h') and \
            conf.CBCheckLib('mysqlclient') and \
            conf.CBCheckFunc('mysql_real_connect_start'):
        env.CBDefine('HAVE_MARIADB')
        env.cb_enabled.add('mariadb')

    # Boost
    env.AppendUnique(CPPPATH = [''])

    # clock_gettime() needed by boost iterprocess
    if env['PLATFORM'] == 'posix' and int(env.get('cross_osx', 0)) == 0 \
            and not conf.CBCheckFunc('clock_gettime'):
        conf.CBRequireLib('rt')
        conf.CBRequireFunc('clock_gettime')

    if with_openssl: conf.CBConfig('openssl', False, version = '1.0.0')
    conf.CBConfig('chakra', False)
    conf.CBConfig('v8', False)

    if env['PLATFORM'] == 'win32' or int(env.get('cross_mingw', 0)):
        if not conf.CBCheckLib('ws2_32'): conf.CBRequireLib('wsock32')
        conf.CBCheckLib('winmm')
        conf.CBRequireLib('setupapi')

    else: conf.CBConfig('pthreads')

    # OSX frameworks
    if env['PLATFORM'] == 'darwin' or int(env.get('cross_osx', 0)):
        if not (conf.CheckOSXFramework('CoreServices') and
                conf.CheckOSXFramework('IOKit') and
                conf.CheckOSXFramework('CoreFoundation')):
            raise SCons.Errors.StopError(
                'Need CoreServices, IOKit & CoreFoundation frameworks')

    conf.CBConfig('valgrind', False)

    # Debug
    if env.get('debug', 0):
        if conf.CBCheckCHeader('execinfo.h') and \
                conf.CBCheckCHeader('bfd.h') and \
                conf.CBCheckLib('iberty') and conf.CBCheckLib('bfd'):
            env.CBDefine('HAVE_CBANG_BACKTRACE')

        elif env.get('backtrace_debugger', 0):
            raise SCons.Errors.StopError(
                'execinfo.h, bfd.h and libbfd needed for backtrace_debuger')

        env.CBDefine('DEBUG_LEVEL=' + str(env.get('debug_level', 1)))


def configure(conf):
    env = conf.env

    home = GetHome() + '/../..'
    env.AppendUnique(CPPPATH = [home + '/src', home + '/include',
                                home + '/src/boost'])
    env.AppendUnique(LIBPATH = [home + '/lib'])

    if not env.CBConfigEnabled('cbang-deps'):
        conf.CBConfig('cbang-deps', local = False)

    conf.CBRequireLib('cbang-boost')
    conf.CBRequireLib('cbang')
    conf.CBRequireCXXHeader('cbang/Exception.h')
    env.CBDefine('HAVE_CBANG')


def generate(env):
    env.CBAddConfigTest('cbang', configure)
    env.CBAddConfigTest('cbang-deps', configure_deps)

    env.CBAddVariables(
        BoolVariable('backtrace_debugger', 'Enable backtrace debugger', 0),
        ('debug_level', 'Set log debug level', 1))

    env.CBLoadTools('''sqlite3 openssl pthreads valgrind osx zlib bzip2
        XML chakra v8 event re2'''.split(), GetHome() + '/..')


def exists(env):
    return 1
