from SCons.Script import *


def configure(conf):
    env = conf.env

    if env['PLATFORM'] == 'win32' or int(env.get('cross_mingw', 0)):
        if not config.CBCheckLib('ws2_32'): conf.CBRequireLib('wsock32')
    else:
        conf.CBConfig('pthreads')
        conf.CBCheckLib('dl')

    conf.CBCheckHome('libsqlite', ['', '/include'], ['', '/lib'])
    conf.CBRequireLib('sqlite3')
    conf.CBRequireHeader('sqlite3.h')
    conf.CBRequireFunc('sqlite3_backup_init')

    env.CBDefine('HAVE_LIBSQLITE')


def generate(env):
    env.CBAddConfigTest('sqlite3', configure)
    env.CBLoadTool('pthreads')


def exists():
    return 1

