'''
Builds an OSX single application package
'''

import os
import shutil

from SCons.Script import *


def build_function(target, source, env):
    # Create package build dir
    build_dir = 'build/pkg'
    if os.path.exists(build_dir): shutil.rmtree(build_dir)

    # Make root dir
    root_dir = os.path.join(build_dir, 'root')
    os.makedirs(root_dir, 0775)

    # Apps
    if 'pkg_apps' in env:
        d = os.path.join(root_dir, 'Applications')
        os.makedirs(d, 0775)
        env.InstallFiles('pkg_apps', d, None)

    # Other files
    if 'pkg_files' in env:
        env.InstallFiles('pkg_files', root_dir)

    # Build package command
    pkg_dir = '%s.pkg' % env.get('package_name')
    cmd = ['${PACKAGEMAKER}',
           '--title', env.get('summary'),
           '--id', env.get('pkg_id', env.get('app_id', '') + '.pkg'),
           '--version', env.get('version'),
           '--install-to', env.get('pkg_install_to', '/'),
           '--out', pkg_dir]
    if not 'pkg_doc' in env: cmd += ['--root',  root_dir]

    # Resources
    if 'pkg_resources' in env:
        d = os.path.join(build_dir, 'Resources')
        os.makedirs(d, 0775)
        env.InstallFiles('pkg_resources', d)
        cmd += ['--resources', d]

    for i in 'info scripts certificate doc target domain filter'.split():
        if 'pkg_' + i in env: cmd += ['--' + i, env.get('pkg_' + i)]
    for i in 'no-recommend no-relocate root-volume-only discard-forks temp-root'.split():
        if env.get('pkg_' + i.replace('-', '_'), False): cmd += ['--' + i]
    env.RunCommand(cmd)

    # Zip results
    env.ZipDir(str(target[0]), pkg_dir)


def configure(conf):
    env = conf.env

    env['PACKAGEMAKER'] = 'packagemaker'

    bld = Builder(action = build_function,
                  source_factory = SCons.Node.FS.Entry,
                  source_scanner = SCons.Defaults.DirScanner)
    env.Append(BUILDERS = {'Pkg' : bld})

    return True
