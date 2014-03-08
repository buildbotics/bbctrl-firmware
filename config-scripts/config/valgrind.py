import os
from SCons.Script import *

def configure(conf):
    env = conf.env

    if conf.CheckCHeader('valgrind/valgrind.h'):
        env.AppendUnique(CPPDEFINES = ['HAVE_VALGRIND_H'])

    if conf.CheckCHeader('valgrind/drd.h'):
        env.AppendUnique(CPPDEFINES = ['HAVE_VALGRIND_DRD_H'])

    return True
