from SCons.Script import *
import config


def configure(conf):
    config.check_home(conf, 'zlib', lib_suffix = ['', '/lib'],
                      inc_suffix = ['/src', '/include'])

    config.require_header(conf, 'zlib.h')
    config.require_lib(conf, 'z')
