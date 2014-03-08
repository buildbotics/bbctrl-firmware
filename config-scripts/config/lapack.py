import sys
from SCons.Script import *
import config


def CheckLAPACK(context):
    env = context.env
    conf = context.sconf

    context.Message('Checking for LAPACK...\n')

    config.check_home(conf, 'lapack')

    if env['PLATFORM'] in ['posix', 'darwin']:
        # G2C
        config.check_home(conf, 'g2c')
        config.check_lib(conf, 'g2c')

        # GFortran
        config.check_home(conf, 'gfortran')
        config.check_lib(conf, 'gfortran')

    # BLAS
    config.check_home(conf, 'blas')
    if not config.check_lib(conf, 'blas-3'):
        config.check_lib(conf, 'blas')

    # LAPACK
    if (config.check_lib(conf, 'lapack-3') or
        config.check_lib(conf, 'lapack')):

        env.AppendUnique(CPPDEFINES = ['HAVE_LAPACK'])
        context.Result(True)
        return True

    context.Result(False)
    return False


def configure(conf):
    conf.AddTest('CheckLAPACK', CheckLAPACK)
    return conf.CheckLAPACK()
