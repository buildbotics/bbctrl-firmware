from SCons.Script import *
import config


def configure(conf):
    config.check_home(conf, 'v8')

    if conf.env['PLATFORM'] == 'win32':
        if not config.check_lib(conf, 'winmm'): return False

    if not config.check_cxx_header(conf, 'v8.h'): return False

    if config.check_lib(conf, 'v8'): return True
    if not config.check_lib(conf, 'v8_snapshot'): return False
    return config.check_lib(conf, 'v8_base') or \
        config.check_lib(conf, 'v8_base.ia32')
