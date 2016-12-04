from SCons.Script import *
import platform


def configure(conf):
    conf.CBCheckHome('chakra_core', lib_suffix = ['/'],
                     inc_suffix = ['/lib/Jsrt', '/include'])

    conf.CBRequireCXXHeader('ChakraCore.h')

    if conf.env['PLATFORM'] != 'win32':
        conf.CBRequireLib('stdc++')
        conf.CBRequireLib('m')

    conf.CBCheckLib('icuuc')
    conf.CBCheckLib('unwind-x86_64')

    if int(conf.env.get('static')):
        conf.CBCheckLib('icudata')
        conf.CBCheckLib('unwind')
        conf.CBCheckLib('lzma')

    conf.CBRequireLib('ChakraCore')

    conf.CBRequireFunc('JsCreateRuntime')

    conf.env.CBDefine('HAVE_CHAKRA')


def generate(env):
    env.CBAddConfigTest('chakra', configure)


def exists():
    return 1
