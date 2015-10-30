# Setup
import os
env = Environment(ENV = os.environ)
try:
    env.Tool('config', toolpath = [os.environ.get('CBANG_HOME')])
except Exception, e:
    raise Exception, 'CBANG_HOME not set?\n' + str(e)

env.CBLoadTools('compiler cbang dist resources build_info packager')
conf = env.CBConfigure()

# Settings
name = 'bbmc'

# Version
version = '0.0.1'
major, minor, revision = version.split('.')

# Config vars
env.Replace(PACKAGE_VERSION = version)
env.Replace(RESOURCES_NS = 'bbmc')
env.Replace(BUILD_INFO_NS = 'bbmc::BuildInfo')

if not env.GetOption('clean') and not 'package' in COMMAND_LINE_TARGETS:
    conf.CBConfig('compiler')

    conf.CBConfig('cbang')
    env.CBDefine('USING_CBANG') # Using CBANG macro namespace

    conf.CBRequireLib('re2')
    conf.CBRequireCXXHeader('re2/re2.h')

conf.Finish()

# Program
Export('env name')
prog, lib = \
    SConscript('src/%s.scons' % name, variant_dir = 'build', duplicate = 0)
Default(prog)

# Clean
Clean(prog, ['build', 'config.log'])

# Dist
docs = ['README.md']
tar = env.TarBZ2Dist(name, docs + [prog])
Alias('dist', tar)
AlwaysBuild(tar)

description = \
'''
Buildbotics Machine Controller
'''

short_description = description


if 'package' in COMMAND_LINE_TARGETS:
    pkg = env.Packager(
        name,
        version = version,
        maintainer = 'Joseph Coffland <joseph@cauldrondevelopment.com>',
        vendor = 'Buildbotics LLC',
        url = 'http://github.com/buildbotics/machine-controller/',
        license = 'copyright',
        bug_url = 'https://github.com/buildbotics/machine-controller/issues',
        summary = 'Buildbotics Machine Controller',
        description = description,
        prefix = '/usr',

        documents = docs,
        programs = [str(prog[0])],
        init_d = [['scripts/' + name + '.init.d', name]],
        changelog = 'ChangeLog',

        deb_directory = 'debian',
        deb_section = 'science',
        deb_depends = 'debconf | debconf-2.0, libc6, bzip2, zlib1g',
        deb_pre_depends = 'adduser, ssl-cert',
        deb_priority = 'optional',
        )

    AlwaysBuild(pkg)
    env.Alias('package', pkg)

    f = None
    try:
        f = open('package-description.txt', 'w')
        f.write(short_description.strip())
    finally:
        if f is not None: f.close()
