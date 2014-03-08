import sys
from SCons.Script import *
import config

deps = ['expat']

def configure(conf):
    env = conf.env

    have_xml_parser = False

    # libexpat
    if config.configure('expat', conf, False):
        have_xml_parser = True

    else: # Glib 2.0
        env.ParseConfig('pkg-config --cflags --libs glib-2.0')
        env.ParseConfig('pkg-config --cflags --libs gthread-2.0')
        if conf.CheckCHeader('glib.h'):
            env.AppendUnique(CPPDEFINES = ['HAVE_GLIB'])
            have_xml_parser = True

    if not have_xml_parser:
        raise Exception, 'Need either libexpat or glib2.0 XML parser'

    return have_xml_parser
