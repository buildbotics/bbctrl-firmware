from SCons.Script import *
import platform


def configure(conf):
    conf.CBCheckHome('chakra_core', lib_suffix = ['/'], inc_suffix = '/lib')
    conf.CBRequireCHeader('Jsrt/ChakraCore.h')
    conf.CBRequireLib('stdc++')
    conf.CBRequireLib('m')
    conf.CBCheckLib('icuuc')
    conf.CBCheckLib('unwind-x86_64')
    conf.CBRequireLib('ChakraCore')
    conf.CBRequireFunc('JsCreateRuntime');

def generate(env):
    env.CBAddConfigTest('chakra', configure)


def exists():
    return 1
