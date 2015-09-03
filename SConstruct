import os

# Version
version = '0.0.1'
libversion = '0'
major, minor, revision = version.split('.')

# Setup
env = Environment(ENV = os.environ,
                  TARGET_ARCH = os.environ.get('TARGET_ARCH', 'x86'))
env.Tool('config', toolpath = ['.'])
env.CBAddVariables(
    BoolVariable('staticlib', 'Build a static library', True),
    BoolVariable('sharedlib', 'Build a shared library', False),
    ('soname', 'Shared library soname', 'libcbang%s.so' % libversion),
    PathVariable('prefix', 'Install path prefix', '/usr/local',
                 PathVariable.PathAccept),
    BoolVariable('with_openssl', 'Build with OpenSSL support', True),
    ('force_local', 'List of 3rd party libs to be built locally', ''),
    ('disable_local', 'List of 3rd party libs not to be built locally', ''))
env.CBLoadTools('dist packager compiler cbang build_info')
env.Replace(PACKAGE_VERSION = version)
conf = env.CBConfigure()

# Build Info
env.Replace(PACKAGE_VERSION = version)
env.Replace(BUILD_INFO_NS = 'cb::BuildInfo')


# Dist
if 'dist' in COMMAND_LINE_TARGETS:
    env.__setitem__('dist_build', '')

    # Only files checked in to Subversion
    lines = os.popen('svn status -v').readlines()
    lines = filter(lambda l: len(l) and l[0] in 'MA ', lines)
    files = map(lambda l: l.split()[-1], lines)
    files = filter(lambda f: not os.path.isdir(f), files)

    tar = env.TarBZ2Dist('libcbang' + libversion, files)
    Alias('dist', tar)
    Return()


# Configure
if not env.GetOption('clean'):
    conf.CBConfig('compiler')
    conf.CBConfig('cbang-deps', with_openssl = env['with_openssl'])
    env.CBDefine('USING_CBANG') # Using CBANG macro namespace

# Local includes
env.Append(CPPPATH = ['#/src', '#/include'])

# Build third-party libs
force_local = env.CBBuildSetRegex(env.get('force_local', ''))
disable_local = env.CBBuildSetRegex(env.get('disable_local', ''))
Export('env conf')
for lib in 'zlib bzip2 sqlite3 expat boost libevent re2'.split():
    if disable_local.match(lib): continue
    if not env.CBConfigEnabled(lib) or force_local.match(lib):
        Default(SConscript('src/%s/SConscript' % lib,
                           variant_dir = 'build/' + lib))

# Source
subdirs = [
    '', 'script', 'xml', 'util', 'debug', 'config', 'pyon', 'os', 'http',
    'macro', 'log', 'iostream', 'time', 'enum', 'packet', 'net', 'buffer',
    'socket', 'tar', 'io', 'geom', 'parse', 'task', 'json',
    'jsapi', 'db', 'auth']

if env.CBConfigEnabled('openssl'): subdirs.append('openssl')
if env.CBConfigEnabled('v8'): subdirs.append('js')
if env.CBConfigEnabled('mariadb'): subdirs.append('db/maria')
if env.CBConfigEnabled('libevent'): subdirs.append('event')

src = []
for dir in subdirs:
    dir = 'src/cbang/' + dir
    src += Glob(dir + '/*.c')
    src += Glob(dir + '/*.cpp')


conf.Finish()


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
Clean(libs, 'build lib include config.log cbang-config.pyc package.txt'
      .split() + Glob('config/*.pyc'))


# Install
prefix = env.get('prefix')
install = [env.Install(dir = prefix + '/lib', source = libs)]

for dir in subdirs:
    files = Glob('src/cbang/%s/*.h' % dir)
    files += Glob('src/cbang/%s/*.def' % dir)
    install.append(env.Install(dir = prefix + '/include/cbang/' + dir,
                               source = files))

docs = ['README.md', 'COPYING']
install.append(env.Install(dir = prefix + '/share/doc/cbang', source = docs))

env.Alias('install', install)


if 'package' in COMMAND_LINE_TARGETS:
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
