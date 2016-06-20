'''Builds an OSX single application package'''

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

    # pkg_id
    if 'app_id' in env and not 'pkg_id' in env:
        env.Replace(pkg_id = env.get('app_id') + '.pkg')

    # pkgbuild command
    cmd = ['${PKGBUILD}',
           '--root', root_dir,
           '--id', env.get('pkg_id'),
           '--version', env.get('version'),
           '--install-location', env.get('pkg_install_to', '/'),
           ]
    if 'pkg_scripts' in env: cmd += ['--scripts', env.get('pkg_scripts')]
    if 'pkg_plist' in env: cmd += ['--component-plist', env.get('pkg_plist')]
    cmd += [build_dir + '/%s.pkg' % env.get('package_name')]

    env.RunCommand(cmd)

    # Filter distribution.xml
    dist = None
    if 'pkg_distribution' in env:
        f = open(env.get('pkg_distribution'), 'r')
        data = f.read()
        f.close()
        dist = build_dir + '/distribution.xml'
        f = open(dist, 'w')
        f.write(data % env)
        f.close()

    # productbuild command
    cmd = ['${PRODUCTBUILD}']
    if dist:
        cmd += ['--distribution', dist]
        cmd += ['--package-path', build_dir]
    else:
        print "WARNING: No distribution specified. Attempting to build " \
          "using --root. Package will not have Resources and will put " \
          "wrong package id in receipts."
        cmd += ['--root', root_dir, env.get('pkg_install_to', '/'),
                '--id', env.get('pkg_id'),
                '--version', env.get('version')]
        if 'pkg_scripts' in env: cmd += ['--scripts', env.get('pkg_scripts')]
    if 'pkg_resources' in env:
        cmd += ['--resources', env.get('pkg_resources')]
    if 'sign_id_installer' in env:
        cmd += ['--sign', env.get('sign_id_installer')]
        if 'sign_keychain' in env:
            cmd += ['--keychain', env.get('sign_keychain')]
    cmd += [str(target[0])]

    env.RunCommand(cmd)


def generate(env):
    env.SetDefault(PKGBUILD = 'pkgbuild')
    env.SetDefault(PRODUCTBUILD = 'productbuild')

    bld = Builder(action = build_function,
                  source_factory = SCons.Node.FS.Entry,
                  source_scanner = SCons.Defaults.DirScanner)
    env.Append(BUILDERS = {'Pkg' : bld})

    return True


def exists():
    return 1
