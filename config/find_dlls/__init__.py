import os
import sys
import re
import subprocess
import glob

from SCons.Script import *


default_exclude = set('''
  advapi32.dll kernel32.dll msvcrt.dll ole32.dll user32.dll ws2_32.dll
  comdlg32.dll gdi32.dll imm32.dll oleaut32.dll shell32.dll winmm.dll
  winspool.drv wldap32.dll ntdll.dll d3d9.dll mpr.dll crypt32.dll dnsapi.dll
  shlwapi.dllversion.dll iphlpapi.dll msimg32.dll setupapi.dll
  opengl32.dll glu32.dll wsock32.dll ws32.dll gdiplus.dll usp10.dll
  comctl32.dll'''.split())


def find_in_path(filename):
    for path in os.environ["PATH"].split(os.pathsep):

        candidate = os.path.join(path.strip('"'), filename)
        if os.path.isfile(candidate): return candidate

        for name in os.listdir(path):
            if name.lower() == filename.lower():
                return os.path.join(path, name)


def find_dlls(env, path, exclude = set()):
    prog = env.get('FIND_DLLS_OBJDUMP')
    cmd = [prog, '-p', path]
    p = subprocess.Popen(cmd, stdout = subprocess.PIPE)
    out, err = p.communicate()

    if p.returncode:
        raise Exception('Call to %s failed: %s' % (prog, err))

    for line in out.splitlines():
        if line.startswith('\tDLL Name: '):
            lib = line[11:].strip().lower()
        else: continue

        if not lib in exclude:
            exclude.add(lib)

            path = find_in_path(lib)
            if path is None:
                if env.get('FIND_DLLS_IGNORE_MISSING'): continue
                raise Exception('Lib "%s" not found' % lib)

            yield path
            for path in find_dlls(env, path, exclude):
                yield path


def FindDLLs(env, source):
    if env.get('FIND_DLLS_DEFAULT_EXCLUDES'): exclude = set(default_exclude)
    else: exclude = set()

    for src in source:
        for path in glob.glob(env.subst(str(src))):
            for dll in find_dlls(env, path, exclude):
                yield dll


def generate(env):
    env.SetDefault(FIND_DLLS_DEFAULT_EXCLUDES = True)
    env.SetDefault(FIND_DLLS_IGNORE_MISSING = True)
    env.SetDefault(FIND_DLLS_OBJDUMP = 'objdump')

    env.AddMethod(FindDLLs)


def exists():
    return True
