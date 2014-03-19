import os, platform, shutil, subprocess

import zipfile

from SCons.Script import *
from SCons.Action import CommandAction

deps = ['nsi', 'pkg', 'distpkg', 'app', 'deb', 'rpm']


def recursive_zip(path, archive, ignores):
    if os.path.isdir(path): names = os.listdir(path)
    else:
        names = [os.path.basename(path)]
        path = os.path.dirname(path)

    if ignores is not None:
        ignored_names = ignores(path, names)
        names = filter(lambda x: not x in ignored_names, names)

    for p in names:
        p = os.path.join(path, p)
        if os.path.isdir(p): recursive_zip(p, archive, ignores)
        else: archive.write(p)


def ZipDir(env, target, source):
    ignores = shutil.ignore_patterns(*env.get('PACKAGE_EXCLUDES'))
    zip = zipfile.ZipFile(target, 'w')
    recursive_zip(source, zip, ignores)
    zip.close()


def write_single_var(stream, name, value, callback = None):
    if callback is not None: value = callback(value)

    stream.write('%s: %s\n' % (name, value))
    

def WriteVariable(self, env, stream, name, var, default = None, callback = None,
                  multi = False):
    if var in env: value = env.get(var)
    elif default is not None: value = default
    else: return

    if multi:
        values = filter(lambda x: x, map(str.strip, value.split(',')))
        for value in values: write_single_var(stream, name, value, callback)

    else: write_single_var(stream, name, value, callback)


def _GetPackageType(env):
    if env['PLATFORM'] == 'win32': return 'exe'

    elif env['PLATFORM'] == 'darwin':
        pkg_type = env.get('pkg_type', None)
        if pkg_type is None or pkg_type in ('single', 'app'): return 'pkg'
        elif pkg_type == 'dist': return 'mpkg'
        else: raise Exception, 'Invalid pkg package type "%s"' % pkg_type

    elif env['PLATFORM'] == 'posix':
        dist = platform.dist()[0].lower()

        if dist in ['debian', 'ubuntu']: return 'deb'
        if dist in ['centos', 'redhat', 'fedora']: return 'rpm'

        # Try to guess
        print 'WARNING: Unrecognized POSIX distribution ' + dist + \
            ', trying to determine package type from filesystem'

        if os.path.exists('/usr/bin/dpkg'): return 'deb'
        if os.path.exists('/usr/bin/rpmbuild'): return 'rpm'

        raise Exception, 'Unsupported POSIX distribution ' + dist

    raise Exception, 'Unsupported package platform %s' % env['PLATFORM']


def GetPackageType(env):
    if env.get('package_type'): return env.get('package_type')
    type = _GetPackageType(env)
    env['package_type'] = type
    return type


def GetPackageArch(env):
    if env.get('package_arch'): return env.get('package_arch')

    type = GetPackageType(env)

    if type == 'deb':
        proc = subprocess.Popen(['/usr/bin/dpkg', '--print-architecture'],
                                stdout = subprocess.PIPE)
        return proc.communicate()[0].strip()

    return platform.machine()
    

def GetPackageName(env, name, without_build = False, type = None):
    name = name.lower()

    if type is None: type = GetPackageType(env)

    if type == 'rpm': seps = ['-', '_']
    else: seps = ['_', '-']

    if env.get('version', False):
        name += seps[0] + env.get('version').replace(seps[0], seps[1])

    if env.get('package_build', False) and not without_build:
        name += seps[0] + env.get('package_build').replace(seps[0], seps[1])

    elif type == 'rpm': name += '-1' # Need release to build RPM

    arch = GetPackageArch(env)
    if arch:
        if type == 'rpm': name += '.' + arch
        else: name += seps[0] + arch # deb and others

    name += '.' + type
    if type in ('app', 'mpkg'): name += '.zip'

    return name



def find_files(path, type = None, ignores = None):
    if not os.path.exists(path): return

    if os.path.isdir(path):
        names = os.listdir(path)

        if ignores is not None:
            ignored_names = ignores(path, names)
            names = filter(lambda x: x in ignored_names, names)

        for name in names:
            for child in find_files(os.path.join(path, name), ignores):
                yield child

    if type is None: yield path
    elif type == 'f':
        if os.path.isfile(path): yield path
    elif type == 'd':
        if os.path.isdir(path): yield path
    elif type == 'l':
        if os.path.islink(path): yield path
    elif type == 'm':
        if os.path.ismount(path): yield path


def FindFiles(env, path, **kwargs):
    return find_files(path, **kwargs)


def resolve_file_map(sources, target, ignores = None, mode = None):
    if not hasattr(sources, '__iter__'): sources = [sources]

    for src in sources:
        if isinstance(src, (list, tuple)) and len(src) in (2, 3):
            src_path = src[0]
            dst_path = os.path.join(target, src[1])
            if len(src) == 3: mode = src[2]
            else: mode = None
        else:
            src_path = src
            dst_path = os.path.join(target, os.path.basename(src))
            mode = None

        if os.path.isdir(src_path):
            names = os.listdir(src_path)
            
            if ignores is not None:
                ignored_names = ignores(src_path, names)
                names = filter(lambda x: not x in ignored_names, names)

            names = map(lambda x: os.path.join(src_path, x), names)

            for item in resolve_file_map(names, dst_path, ignores, mode):
                yield item

        else: yield [src_path, dst_path, mode]


def ResolvePackageFileMap(env, sources, target):
    ignores = shutil.ignore_patterns(*env.get('PACKAGE_EXCLUDES'))
    return resolve_file_map(sources, target, ignores)


def CopyToPackage(env, sources, target, perms = 0644, dperms = 0755):
    ignores = shutil.ignore_patterns(*env.get('PACKAGE_EXCLUDES'))

    for src, dst, mode in resolve_file_map(sources, target, ignores):
        print 'installing "%s" to "%s"' % (src, dst)

        if mode is not None: perms = mode
        dst_dir = os.path.dirname(dst)
        if not os.path.exists(dst_dir): os.makedirs(dst_dir, dperms)
        shutil.copy2(src, dst)
        if perms:
            if os.path.isdir(dst):
                dst = os.path.join(dst, os.path.basename(src))
            os.chmod(dst, perms)


def InstallFiles(env, key, target, perms = 0644, dperms = 0755):
    if key in env: env.CopyToPackage(env.get(key), target, perms, dperms)


def RunCommand(env, cmd):
    print '@', cmd
    CommandAction(cmd).execute(None, [], env)


def WriteStringToFile(env, path, contents):
    if not hasattr(contents, '__iter__'): contents = [contents]

    f = None
    try:
        f = open(path, 'w')
        for line in contents: f.write(line + '\n')
    finally:
        if f is not None: f.close()


def Packager(env, name, **kwargs):
    # Setup env
    kwargs['package_name'] = name
    kwargs['package_name_lower'] = name.lower()
    env = env.Clone()
    env.Replace(**kwargs)

    # Compute package name
    target = GetPackageName(env, name)

    # Write package.txt
    env.WriteStringToFile('package.txt', target)

    # Call OS specific package builder
    type = GetPackageType(env)
    if type == 'exe': return env.Nsis(target, kwargs['nsi'], **kwargs)
    elif type == 'app': return env.App(target, [], **kwargs)
    elif type == 'pkg':
        if not 'app_id' in kwargs: return env.Pkg(target, [], **kwargs)

        name = env.get('package_name')
        app_name = env.GetPackageName(name, type = 'app')

        app = env.App(app_name, [], **kwargs)

        if not 'pkg_apps' in kwargs: kwargs['pkg_apps'] = name + '.app'
        env = env.Clone()
        env.Replace(**kwargs)
        mpkg_name = env.GetPackageName(name, type = 'pkg')
        pkg = env.Pkg(mpkg_name, [], **kwargs)
        Depends(pkg, app)

        return (app, pkg)

    elif type == 'mpkg': return env.DistPkg(target, [], **kwargs)
    elif type == 'deb': return env.Deb(target, [], **kwargs)
    elif type == 'rpm': return env.RPM(target, [], **kwargs)


def generate(env):
    env.CBAddVariables(
            ('package_type', 'Override the package type'),
            ('package_build', 'Set package build name'),
            ('package_clean', 'Clean package build files', False),
            ('package_arch', 'Clean package architecture'),
            )

    env.SetDefault(PACKAGE_EXCLUDES = ['.svn', '.sconsign.dblite',
                                       '.sconf_temp', '*~', '*.o', '*.obj'])

    env.CBLoadTool('nsi')
    env.CBLoadTool('pkg')
    env.CBLoadTool('distpkg')
    env.CBLoadTool('app')
    env.CBLoadTool('deb')
    env.CBLoadTool('rpm')

    env.AddMethod(FindFiles)
    env.AddMethod(WriteVariable)
    env.AddMethod(GetPackageName)
    env.AddMethod(GetPackageType)
    env.AddMethod(GetPackageArch)
    env.AddMethod(CopyToPackage)
    env.AddMethod(InstallFiles)
    env.AddMethod(RunCommand)
    env.AddMethod(ResolvePackageFileMap)
    env.AddMethod(WriteStringToFile)
    env.AddMethod(ZipDir)
    env.AddMethod(Packager)

    return True


def exists():
    return 1
