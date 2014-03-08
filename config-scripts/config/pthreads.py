import os
from SCons.Script import *
import config


def configure(conf):
    env = conf.env

    config.check_home(conf, 'pthreads')
    config.require_header(conf, 'pthread.h')
    config.require_lib(conf, 'pthread')

    env.AppendUnique(CPPDEFINES = ['HAVE_PTHREADS'])
