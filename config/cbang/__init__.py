from SCons.Script import *
import sys


def configure_deps(conf):
    env = conf.env

    conf.CBConfig('zlib')
    conf.CBConfig('bzip2')
    conf.CBConfig('XML', False)
    conf.CBConfig('openssl', version = '1.0.0')
    conf.CBConfig('v8', False)

    if conf.CBConfig('sqlite3', False): env.CBDefine('HAVE_DB')

    conf.CBConfig('boost', version = '1.40',
                  hdrs = ['version', 'iostreams/stream'],
                  libs = ['iostreams', 'system', 'filesystem', 'regex'])

    if env['PLATFORM'] == 'win32':
        conf.CBRequireLib('wsock32')
        conf.CBRequireLib('setupapi')

    else: conf.CBConfig('pthreads')

    # OSX frameworks
    if env['PLATFORM'] == 'darwin':
        env.ConfigOSX()
        if not (conf.CheckOSXFramework('CoreServices') and
                conf.CheckOSXFramework('IOKit') and
                conf.CheckOSXFramework('CoreFoundation')):
            raise Exception, \
                'Need CoreServices, IOKit & CoreFoundation frameworks'

    conf.CBConfig('valgrind')

    # Debug
    if env.get('debug', 0):
        if conf.CBCheckCHeader('execinfo.h') and \
                conf.CBCheckCHeader('bfd.h') and \
                conf.CBCheckLib('iberty') and conf.CBCheckLib('bfd'):
            env.CBDefine('HAVE_CBANG_BACKTRACE')

        elif env.get('backtrace_debugger', 0):
            raise Exception, \
                'execinfo.h, bfd.h and libbfd needed for backtrace_debuger'

        env.CBDefine('DEBUG_LEVEL=' + str(env.get('debug_level', 1)))


def configure(conf):
    env = conf.env
    conf = conf.sconf

    conf.CBConfig('cbang-deps')

    # lib
    home = conf.CBCheckHome()
    if home:
        env.AppendUnique(CPPPATH = [home + '/src'])
        env.AppendUnique(LIBPATH = [home + '/lib'])

    if not (conf.CBCheckLib('cbang') and
            conf.CBCheckCXXHeader('cbang/Exception.h')):
        return False

    env.AppendUnique(CPPDEFINES = 'HAVE_CBANG')


def generate(env):
    env.CBAddConfigTest('cbang', configure)
    env.CBAddConfigTest('cbang-deps', configure_deps)

    env.CBAddVariables(
        BoolVariable('backtrace_debugger', 'Enable backtrace debugger', 0),
        ('debug_level', 'Set log debug level', 1))

    env.CBLoadTools(
        'sqlite3 boost openssl pthreads valgrind osx zlib bzip2 XML v8'.split())


def exists(env):
    return 1
