import os

# Version
version = '0.0.1'
libversion = '0'
major, minor, revision = version.split('.')

# Setup
env = Environment()
env.Tool('config', toolpath = ['.'])
env.CBAddVariables(
    BoolVariable('staticlib', 'Build a static library', True),
    BoolVariable('sharedlib', 'Build a shared library', False),
    ('soname', 'Shared library soname', 'libcbang%s.so' % libversion),
    PathVariable('prefix', 'Install path prefix', '/usr/local',
                 PathVariable.PathAccept))
env.CBLoadTools('dist packager compiler cbang build_info')
env.Replace(PACKAGE_VERSION = version)
conf = env.CBConfigure()

# Build Info
env.Replace(PACKAGE_VERSION = version)
env.Replace(BUILD_INFO_NS = 'cb::BuildInfo')


# Debian
dist_files = []
'''
vars = {'version': version}
for path in Glob('debian/*.in'):
    inFile = None
    outFile = None
    try:
        inFile = open(path, 'r')
        outFile = open(path[:-3], 'w')
        outFile.write(inFile.read() % vars)
        dist_files.append(path[:-3])
    finally:
        if inFile is not None: inFile.close()
        if outFile is not None: outFile.close()
'''

# Dist
if 'dist' in COMMAND_LINE_TARGETS:
    env.__setitem__('dist_build', '')

    # Only files checked in to Subversion
    lines = os.popen('svn status -v').readlines()
    lines = filter(lambda l: len(l) and l[0] in 'MA ', lines)
    files = map(lambda l: l.split()[-1], lines)
    files = filter(lambda f: not os.path.isdir(f), files)

    tar = env.TarBZ2Dist('libcbang' + libversion, files + dist_files)
    Alias('dist', tar)
    Return()


# Configure
if not env.GetOption('clean'):
    conf.CBConfig('compiler')
    conf.CBConfig('cbang-deps')
    env.CBDefine('USING_CBANG') # Using CBANG macro namespace


# Build third-party libs
Export('env conf')
for lib in 'zlib bzip2 sqlite3 expat boost'.split():
    if not env.CBConfigEnabled(lib):
        Default(SConscript('src/%s/SConscript' % lib,
                           variant_dir = 'build/' + lib))

# Local includes
env.Append(CPPPATH = ['#/src'])

conf.Finish()

# Source
subdirs = [
    '', 'script', 'xml', 'util', 'debug', 'config', 'pyon', 'os', 'http',
    'macro', 'log', 'iostream', 'time', 'enum', 'packet', 'net', 'buffer',
    'socket', 'security', 'tar', 'io', 'geom', 'parse', 'task', 'json',
    'jsapi']

if env.CBConfigEnabled('v8'): subdirs.append('js')
if env.CBConfigEnabled('sqlite3'): subdirs.append('db')

src = []
for dir in subdirs:
    dir = 'src/cbang/' + dir
    src += Glob(dir + '/*.c')
    src += Glob(dir + '/*.cpp')


# Build in 'build'
import re
VariantDir('build', 'src')
src = map(lambda path: re.sub(r'^src/', 'build/', str(path)), src)


# Build Info
info = env.BuildInfo('build/build_info.cpp', [])
AlwaysBuild(info)
src.append(info)


# Build
libs = []
if env.get('staticlib'):
    libs.append(env.StaticLibrary('lib/cbang', src))

if env.get('sharedlib'):
    env.Append(SHLIBSUFFIX = '.' + version)
    env.Append(SHLINKFLAGS = '-Wl,-soname -Wl,${soname}')
    libs.append(env.SharedLibrary('lib/cbang' + libversion, src))

for lib in libs: Default(lib)

# Clean
Clean(libs, 'build lib config.log cbang-config.pyc package.txt'.split() +
      Glob('config/*.pyc'))


# Install
prefix = env.get('prefix')
install = [env.Install(dir = prefix + '/lib', source = libs)]

for dir in subdirs:
    files = Glob('src/cbang/%s/*.h' % dir)
    files += Glob('src/cbang/%s/*.def' % dir)
    install.append(env.Install(dir = prefix + '/include/cbang/' + dir,
                               source = files))

docs = ['README', 'COPYING']
install.append(env.Install(dir = prefix + '/share/doc/cbang', source = docs))

env.Alias('install', install)


# .deb Package
if env.GetPackageType() == 'deb':
    arch = env.GetPackageArch()
    pkg = 'libcbang%s_%s_%s.deb' % (libversion, version, arch)
    dev = 'libcbang%s-dev_%s_%s.deb' % (libversion, version, arch)

    env['ENV']['DEB_DEST_DIR'] = '1'
    cmd = env.Command([pkg, dev], libs, 'fakeroot debian/rules binary')
    env.Alias('package', cmd)

    # Write package.txt
    env.WriteStringToFile('package.txt', [pkg, dev])
