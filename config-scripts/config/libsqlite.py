from SCons.Script import *
import config

deps = ['pthreads']


def configure(conf):
    env = conf.env

    if env['PLATFORM'] == 'win32': config.require_lib(conf, 'wsock32')
    else: config.configure('pthreads', conf)

    config.check_home(conf, 'libsqlite', ['', '/include'], ['', '/lib'])
    config.require_lib(conf, 'sqlite3')
    config.require_header(conf, 'sqlite3.h')

    env.AppendUnique(CPPDEFINES = ['HAVE_LIBSQLITE'])
