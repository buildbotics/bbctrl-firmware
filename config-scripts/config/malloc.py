import os
from SCons.Script import *
import config



def add_vars(vars):
    vars.AddVariables(
        ('dmalloc', 'Compile with dmalloc', 0),
        ('tcmalloc', 'Compile with google perfomrance tools malloc', 0),
        ('cwd', 'Compile with libcwd', 0),
        ('efence', 'Compile with electric-fence', 0))


def configure(conf, cxx = True, threads = True):
    env = conf.env

    # See http://dmalloc.com/
    if env.get('dmalloc'):
        libname = 'dmalloc'

        if threads: libname += 'th'
        if cxx: libname += 'cxx'

        if cxx: lang = 'C++'
        else: lang = None

        config.require_lib(conf, libname, language = lang)


    # See http://linux.die.net/man/3/efence
    if env.get('efence'): config.require_lib(conf, 'efence')


    # See http://code.google.com/p/google-perftools
    if env.get('tcmalloc'):
        if conf.CheckCXXHeader('google/tcmalloc.h'):
            env.AppendUnique(CPPDEFINES = ['HAVE_TCMALLOC_H'])

        libname = 'tcmalloc'
        #if env.get('debug'): libname += '_debug'

        config.require_lib(conf, libname)
        config.require_lib(conf, 'unwind')

        if env.get('static') or env.get('mostly_static'):
            env.AppendUnique(LINKFLAGS = ['-Wl,--eh-frame-hdr'])

        env.AppendUnique(CCFLAGS = [
                '-fno-builtin-malloc', '-fno-builtin-calloc',
                '-fno-builtin-realloc', '-fno-builtin-free'])


    # See http://libcwd.sourceforge.net/
    if env.get('cwd'):
        libname = 'cwd'
        if threads:
            libname += '_r'
            env.AppendUnique(CPPDEFINES = ['LIBCWD_THREAD_SAFE'])

        if env.get('debug'):
            env.AppendUnique(CPPDEFINES = ['CWDEBUG'])

        if conf.CheckCXXHeader('libcwd/sys.h'):
            env.AppendUnique(CPPDEFINES = ['HAVE_LIBCWD'])

        config.require_lib(conf, libname)

        # libcwd does not work if libdl is included anywhere earlier
        requrie_lib(conf, 'dl')


    return True
