import sys
import config

deps = ['mkl']


def add_vars(vars):
    vars.AddVariables(
        BoolVariable('fah', 'Set to 1 to build for Folding@home', 0),
        )


def configure_deps(conf):
    env = conf.env

    config.check_home(conf, 'gromacs', inc_suffix = '', lib_suffix = '')

    # Libraries
    for lib in ['nsl', 'm', 'pthread', 'gsl']:
        if config.check_lib(conf, lib):
            env.Append(CPPDEFINES = ['HAVE_LIB' + lib.upper()])


    # MKL
    if config.configure('mkl', conf, False) and conf.CheckCHeader('mkl_dfti.h'):
        env.Append(CPPDEFINES = ['GMX_FFT_MKL'])


    # XDR
    if not conf.CheckCHeader('rpc/xdr.h'):
        env.Append(CPPDEFINES = ['GMX_INTERNAL_XDR'])


    # libxml2
    try:
        env.ParseConfig('xml2-config --libs --cflags')
    except: pass # Ignore errors
    if config.check_lib(conf, 'xml2') and conf.CheckCHeader('libxml/parser.h'):
        env.Append(CPPDEFINES = ['HAVE_LIBXML2'])


    env.Append(CPPDEFINES = ['GMX_DOUBLE'])


def configure(conf):
    env = conf.env

    found = False

    # This is missing from some libxml2 deps
    env.PrependUnique(LIBS = ['z'])

    # Try to use pkg-config
    if not int(env.get('fah', 0)):
        try:
            env.ParseConfig('pkg-config --libs --cflags libgmx_d libmd_d')
            found = True
        except: pass

    # Else try manual configuration
    if not found: configure_deps(conf)

    # Check for Gromacs libraries
    for lib in ['gmx', 'md']:
        if int(env.get('fah', 0)): config.require_lib(conf, lib + '-fah')
        elif not config.check_lib(conf, lib + '_d'):
            config.require_lib(conf, lib)

    # Check for Gromacs header
    if not conf.CheckCHeader('gromacs/tpxio.h'):
        raise Exception, 'Need gromacs/tpxio.h'


    env.Append(CPPDEFINES = 'HAVE_GROMACS')
