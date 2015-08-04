import os
from SCons.Script import *


def configure(conf, cxx = True, threads = True):
    env = conf.env

    # See http://dmalloc.com/
    if env.get('dmalloc'):
        libname = 'dmalloc'

        if threads: libname += 'th'
        if cxx: libname += 'cxx'

        if cxx: lang = 'C++'
        else: lang = None

        conf.CBRequireLib(libname, language = lang)


    # See http://linux.die.net/man/3/efence
    if env.get('efence'): conf.CBRequireLib('efence')


    # See http://code.google.com/p/google-perftools
    if env.get('tcmalloc'):
        if conf.CBCheckCXXHeader('google/tcmalloc.h'):
            env.CBDefine('HAVE_TCMALLOC_H')

        libname = 'tcmalloc'
        #if env.get('debug'): libname += '_debug'

        conf.CBRequireLib(libname)
        env.Append(PREFER_DYNAMIC = [libname])

        env.AppendUnique(CCFLAGS = [
                '-fno-builtin-malloc', '-fno-builtin-calloc',
                '-fno-builtin-realloc', '-fno-builtin-free'])


    # See http://libcwd.sourceforge.net/
    if env.get('cwd'):
        libname = 'cwd'
        if threads:
            libname += '_r'
            env.CBDefine('LIBCWD_THREAD_SAFE')

        if env.get('debug'): env.CBDefine('CWDEBUG')

        if conf.CBCheckCXXHeader('libcwd/sys.h'):
            env.CBDefine('HAVE_LIBCWD')

        conf.CBRequireLib(libname)

        # libcwd does not work if libdl is included anywhere earlier
        conf.CBRequireLib('dl')


def generate(env):
    env.CBAddConfigTest('malloc', configure)

    env.CBAddVariables(
        ('dmalloc', 'Compile with dmalloc', 0),
        ('tcmalloc', 'Compile with google perfomrance tools malloc', 0),
        ('cwd', 'Compile with libcwd', 0),
        ('efence', 'Compile with electric-fence', 0))


def exists():
    return 1
