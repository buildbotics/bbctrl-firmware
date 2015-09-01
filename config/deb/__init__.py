import os
import shutil
import string
import math

from SCons.Script import *
from SCons.Action import CommandAction


def get_total_file_size(path):
    total = 0
    for root, dirs, files in os.walk(path):
        for name in files:
            total += os.path.getsize(os.path.join(root, name))
    return total


def replace_underscore(s):
    return s.replace('_', '-')


def write_control(path, env, installed_size):
    f = None
    try:
        f = open(path, 'w')

        write_var = env.WriteVariable
        write_var(env, f, 'Package', 'package_name_lower', None,
                  replace_underscore)
        write_var(env, f, 'Version', 'version', None, replace_underscore)
        write_var(env, f, 'Standards-Version', 'deb_standards_version')
        write_var(env, f, 'Maintainer', 'maintainer')
        write_var(env, f, 'Priority', 'deb_priority')
        write_var(env, f, 'Section', 'deb_section')
        write_var(env, f, 'Bugs', 'bug_url')
        write_var(env, f, 'Homepage', 'url')
        write_var(env, f, 'Essential', 'deb_essential')
        write_var(env, f, 'Distribution', 'deb_distribution')
        write_var(env, f, 'Depends', 'deb_depends')
        write_var(env, f, 'Pre-Depends', 'deb_pre_depends')
        write_var(env, f, 'Recommends', 'deb_recommends')
        write_var(env, f, 'Suggests', 'deb_suggests')
        write_var(env, f, 'Breaks', 'deb_breaks')
        write_var(env, f, 'Conflicts', 'deb_conflicts')
        write_var(env, f, 'Provides', 'deb_provides')
        write_var(env, f, 'Replaces', 'deb_replaces')
        write_var(env, f, 'Enhances', 'deb_enhances')
        write_var(env, f, 'Architecture', 'package_arch', env.GetPackageArch())

        f.write('Installed-Size: %d\n' % math.ceil(installed_size / 1024))

        write_var(env, f, 'Description', 'summary')
        for line in env.get('description').split('\n'):
            if line.strip() == '': line = '.'
            f.write(' %s\n' % line)


    finally:
        if f is not None: f.close()


def install_files(env, key, target, perms = 0644, dperms = 0755):
    if key in env: env.CopyToPackage(env.get(key), target, perms, dperms)


def get_files(env, key, target):
    if key in env: return env.ResolvePackageFileMap(env.get(key), target)


def build_function(target, source, env):
    target = str(target[0])
    name = env.get('package_name_lower')

    # Create package build dir
    build_dir = 'build/%s-deb' % name
    if os.path.exists(build_dir): shutil.rmtree(build_dir)
    os.makedirs(build_dir, 0755)

    # Copy user control files
    shutil.copytree(env.get('deb_directory'), build_dir + '/DEBIAN',
                    ignore = shutil.ignore_patterns('.svn', '*~'))

    # Copy files into package
    install_files(env, 'documents', build_dir + '/usr/share/doc/' + name)
    install_files(env, 'programs', build_dir + '/usr/bin', 0755)
    install_files(env, 'scripts', build_dir + '/usr/bin', 0755)
    install_files(env, 'desktop_menu', build_dir + '/usr/share/applications')
    install_files(env, 'init_d', build_dir + '/etc/init.d', 0755)
    install_files(env, 'config', build_dir + '/etc/' + name)
    install_files(env, 'icons', build_dir + '/usr/share/pixmaps')
    install_files(env, 'platform_independent', build_dir + '/usr/share/' + name)

    # Dirs
    docs_dir =  build_dir + '/usr/share/doc/' + name

    # Create conffiles
    filename = build_dir + '/DEBIAN/conffiles'
    f = None
    try:
        conffiles = get_files(env, 'init_d', '/etc/init.d')
        if conffiles is not None:
            for src, dst, mode in conffiles:
                if f is None: f = open(filename, 'w')
                f.write(dst + '\n')
    finally:
        if f is not None:
            f.close()
            os.chmod(filename, 0644)

    # Debian changelog
    changelog = build_dir + '/DEBIAN/changelog'
    if os.path.exists(changelog):
        dest = docs_dir + '/changelog.Debian'

        shutil.move(changelog, dest)
        cmd = 'gzip -9 ' + dest
        CommandAction(cmd).execute(dest + '.gz', [dest], env)

    # ChangeLog
    if 'changelog' in env:
        changelog = env.get('changelog')
        for changelog in [docs_dir + '/' + changelog, changelog]:
            if os.path.exists(changelog):
                dest = docs_dir + '/changelog'

                if changelog != dest: shutil.move(changelog, dest)

                # Compress
                cmd = 'gzip -9 ' + dest
                CommandAction(cmd).execute(dest + '.gz', [dest], env)

                break

    # Strip exectuables
    if not int(env.get('debug')):
        for src, dst, mode in get_files(env, 'programs',
                                        build_dir + '/usr/bin'):
            CommandAction('strip ' + dst).execute(dst, [dst], env)

    # Create debian control
    total_size = get_total_file_size(build_dir)
    write_control(build_dir + '/DEBIAN/control', env, total_size)

    # Execute
    if 'deb_execute' in env:
        print env.get('deb_execute')
        cmd = string.Template(env.get('deb_execute'))
        cmd = cmd.substitute(package_root = os.path.realpath(build_dir))
        CommandAction(cmd).execute(None, [None], env)

    # Fix permissions
    for path in env.FindFiles(build_dir + '/DEBIAN'):
        mode = os.stat(path).st_mode & 0700
        mode = (mode | (mode >> 3) | (mode >> 6)) & 0755
        os.chmod(path, mode)

    # Build the package
    cmd = 'fakeroot dpkg-deb -b %s .' % build_dir
    CommandAction(cmd).execute(target, [build_dir], env)

    # Rename if necessary
    if 'package_build' in env:
        build = replace_underscore(env.get('package_build'))
        name = target.replace(build + '_', '')
        if os.path.exists(target): os.unlink(target)
        shutil.move(name, target)


def generate(env):
    bld = Builder(action = build_function)
    env.Append(BUILDERS = {'Deb' : bld})

    return True


def exists():
    return 1

