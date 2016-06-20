#!env python
import sys
import os
import getopt
from distutils import sysconfig


pyver = sysconfig.get_config_var('VERSION')
getvar = sysconfig.get_config_var

flags = ['-I' + sysconfig.get_python_inc(),
         '-I' + sysconfig.get_python_inc(plat_specific=True)]
print ' '.join(flags)

libs = getvar('LIBS').split() + getvar('SYSLIBS').split()
libs.append('-lpython' + pyver)
if not getvar('Py_ENABLE_SHARED'):
    libs.insert(0, '-L' + getvar('LIBPL'))
print ' '.join(libs)
