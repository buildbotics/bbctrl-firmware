import os
import sys
import re
import subprocess
import glob

from SCons.Script import *


default_exclude = set([re.compile(r'^.*\\system32\\.*$')])


def find_in_path(filename):
    for path in os.environ["PATH"].split(os.pathsep):

        candidate = os.path.join(path.strip('"'), filename)
        if os.path.isfile(candidate): return candidate

        for name in os.listdir(path):
            if name.lower() == filename.lower():
                return os.path.join(path, name)


def find_dlls(env, path, exclude = set()):
    if env['PLATFORM'] == 'win32':
        prog = env.get('FIND_DLLS_DUMPBIN')
        cmd = [prog, '/DEPENDENTS', '/NOLOGO', path]

    else:
        prog = env.get('FIND_DLLS_OBJDUMP')
        cmd = [prog, '-p', path]


    p = subprocess.Popen(cmd, stdout = subprocess.PIPE)
    out, err = p.communicate()

    if p.returncode:
        raise Exception('Call to %s failed: %s' % (prog, err))

    for line in out.splitlines():
        if line.startswith('\tDLL Name: '): lib = line[11:]
        else:
            m = re.match(r'^\s+([^\s.]+\.[dD][lL][lL])\s*$', line)
            if m: lib = m.group(1)
            else: continue

        lib = lib.strip().lower()

        if not lib in exclude:
            exclude.add(lib)

            path = find_in_path(lib)
            if path is None:
                if env.get('FIND_DLLS_IGNORE_MISSING'): continue
                raise Exception('Lib "%s" not found' % lib)

            ok = True
            for pat in exclude:
                if isinstance(re.compile(''), pat) and pat.match(path):
                    ok = False
                    break

            if not ok: continue

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
    env.SetDefault(FIND_DLLS_DUMPBIN = 'dumpbin')

    env.AddMethod(FindDLLs)


def exists():
    return True
