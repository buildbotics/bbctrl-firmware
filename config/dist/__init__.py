import os
import sys
import tarfile
import re
import platform
import time
from SCons.Script import *


def find_files(path, exclude = None):
    if not os.path.exists(path): return []
    if exclude:
        dir, filename = os.path.split(path)
        if exclude.match(filename): return []
    if not os.path.isdir(path): return [path]

    files = []
    for f in os.listdir(path):
        files += find_files(path + '/' + f, exclude)

    return files


def modify_targets(target, source, env):
    if env.get('debug', False): mode = 'debug'
    else: mode = 'release'

    dist_ver = env.get('dist_version', env.subst('$PACKAGE_VERSION'))

    vars = {
        'machine' : platform.machine(),
        'bits' : platform.architecture()[0],
        'system' : platform.system(),
        'release' : platform.release(),
        'version' : dist_ver,
        'date' : time.strftime('%Y%m%d'),
        'mode' : mode,
        }

    build = env.get('dist_build')
    if len(build) and build[0] not in '-_': build = '_' + build
    if len(dist_ver): build = '_' + dist_ver + build

    target = ((str(target[0]) + build) % vars % env._dict) + '.tar.bz2'

    return [target.lower(), source]


def build_function(target, source, env):
    target = str(target[0])

    # Write 'dist.txt'
    f = None
    try:
        f = open('dist.txt', 'w')
        f.write(target)
    finally:
        if f is not None: f.close()

    dist_name = os.path.splitext(os.path.splitext(target)[0])[0]

    tar = tarfile.open(target, mode = 'w:bz2')

    exclude_pats = env.get('DIST_EXCLUDES')
    exclude_re = re.compile('^((' + ')|('.join(exclude_pats) + '))$')

    for src in map(str, source):
        for file in find_files(src, exclude_re):
            tar.add(file, dist_name + '/' + file)


def generate(env):
    env.SetDefault(DIST_EXCLUDES = [r'\.svn', r'\.sconsign.dblite', r'.*\.obj',
                                    r'\.sconf_temp', r'.*~', r'.*\.o'])

    env.CBAddVariables(
            ('dist_version', 'Set dist file version', None),
            ('dist_build', 'Set dist file build info', '-%(bits)s-%(mode)s'))

    bld = Builder(action = build_function,
                  source_factory = SCons.Node.FS.Entry,
                  source_scanner = SCons.Defaults.DirScanner,
                  emitter = modify_targets)
    env.Append(BUILDERS = {'TarBZ2Dist' : bld})

    return True


def exists():
    return 1
