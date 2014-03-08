# Configure system boilerplate
import os, sys, glob
sys.path.append(os.environ.get('CONFIG_SCRIPTS_HOME', './config-scripts'))
import config
import collections

# Version
version = '0.0.1'
libversion = '0'
major, minor, revision = version.split('.')

# Setup
vars = [
    BoolVariable('staticlib', 'Build a static library', True),
    BoolVariable('sharedlib', 'Build a shared library', False),
    ('soname', 'Shared library soname', 'libcbang%s.so' % libversion),
    PathVariable('prefix', 'Install path prefix', '/usr/local',
                 PathVariable.PathAccept),
    ]
env = config.make_env(['compiler', 'dist', 'build_info', 'packager'], vars)

# Configure
conf = Configure(env)

# Build Info
config.configure('build_info', conf, namespace = 'cb::BuildInfo',
                 version = version)

# Packaging
config.configure('dist', conf, version = version)
config.configure('packager', conf)


# Debian
dist_files = []
'''
vars = {'version': version}
for path in glob.glob('debian/*.in'):
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
    #AlwaysBuild(tar)
    Return()


# Configure
if not env.GetOption('clean'):
    # Configure compiler
    config.configure('compiler', conf)

    # Dependencies
    lib = config.load_conf_module('cbang', '.')
    lib.configure_deps(conf)

    # Using CBANG macro namespace
    env.Append(CPPDEFINES = ['USING_CBANG'])


# Local includes
env.Append(CPPPATH = ['#/src'])

conf.Finish()

# Source
subdirs = [
    '', 'script', 'xml', 'util', 'debug', 'config', 'pyon', 'os', 'http',
    'macro', 'log', 'iostream', 'time', 'enum', 'packet', 'net', 'buffer',
    'socket', 'security', 'tar', 'io', 'geom', 'parse', 'task', 'json',
    'jsapi']

if config.enabled.get('v8', False): subdirs.append('js')
if config.enabled.get('libsqlite', False): subdirs.append('db')

src = []
for dir in subdirs:
    dir = 'src/cbang/' + dir
    src += glob.glob(dir + '/*.c')
    src += glob.glob(dir + '/*.cpp')


# Build in 'build'
import re
VariantDir('build', 'src')
src = map(lambda path: re.sub(r'^src/', 'build/', path), src)


# Build Info
info = env.BuildInfo('build/build_info.cpp', [])
AlwaysBuild(info)
src.append(info)


# Build
libs = []
if env.get('staticlib'):
#    libs.append(env.StaticLibrary('cbang' + libversion, src))
    libs.append(env.StaticLibrary('cbang', src))
if env.get('sharedlib'):
    env.Append(SHLIBSUFFIX = '.' + version)
    env.Append(SHLINKFLAGS = '-Wl,-soname -Wl,${soname}')
    libs.append(env.SharedLibrary('cbang' + libversion, src))
for lib in libs: Default(lib)


# Install
prefix = env.get('prefix')
install = [env.Install(dir = prefix + '/lib', source = libs)]

for dir in subdirs:
    files = glob.glob('src/cbang/%s/*.h' % dir)
    files += glob.glob('src/cbang/%s/*.def' % dir)
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
