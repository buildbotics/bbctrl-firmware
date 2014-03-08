from SCons.Script import *
import config


def configure(conf):
    config.check_home(conf, 'libbzip2', lib_suffix = ['', '/lib'],
                      inc_suffix = ['/src', '/include'])

    config.require_header(conf, 'bzlib.h')
    config.require_lib(conf, 'bz2')
