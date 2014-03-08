import os
import shutil

from SCons.Script import *
from SCons.Action import CommandAction


def replace_dash(s):
    return s.replace('-', '_')


def write_spec_text_section(f, env, name, var):
    if var in env:
        f.write('%%%s\n%s\n\n' % (name, env.get(var).strip()))


def write_spec_script(f, env, name, var):
    if var in env:
        script = env.get(var)

        input = None
        try:
            input = open(script, 'r')
            contents = input.read().strip()

        finally:
            if input is not None: input.close()

        f.write('%%%s\n%s\n\n' % (name, contents))


def install_files(f, env, key, build_dir, path, prefix = None, perms = None,
                  dperms = 0755):
    if perms is None: perms = 0644

    if key in env:
        target = build_dir + path

        # Copy
        env.CopyToPackage(env.get(key), target, perms, dperms)

        # Write files list
        for src, dst, mode in env.ResolvePackageFileMap(env.get(key), target):
            if prefix is not None: f.write(prefix + ' ')
            f.write(dst[len(build_dir):] + '\n')
            

def build_function(target, source, env):
    name = env.get('package_name_lower')

    # Create package build dir
    build_dir = 'build/%s-RPM' % name
    if os.path.exists(build_dir): shutil.rmtree(build_dir)
    os.makedirs(build_dir)

    # Create the SPEC file
    spec_file = 'build/%s.spec' % name
    f = None
    try:
        f = open(spec_file, 'w')

        # Create the preamble
        write_var = env.WriteVariable
        write_var(env, f, 'Summary', 'summary')
        write_var(env, f, 'Name', 'package_name_lower', None, replace_dash)
        write_var(env, f, 'Version', 'version', None, replace_dash)
        write_var(env, f, 'Release', 'package_build', '1', replace_dash)
        write_var(env, f, 'License', 'rpm_license')
        write_var(env, f, 'Group', 'rpm_group')
        write_var(env, f, 'URL', 'url')
        write_var(env, f, 'Vendor', 'vendor')
        write_var(env, f, 'Packager', 'maintainer')
        write_var(env, f, 'Icon', 'icon')
        write_var(env, f, 'Prefix', 'prefix')
        #write_var(env, f, 'BuildArch', 'package_arch', env.GetPackageArch())
        write_var(env, f, 'Provides', 'rpm_provides', multi = True)
        write_var(env, f, 'Conflicts', 'rpm_conflicts', multi = True)
        write_var(env, f, 'Obsoletes', 'rpm_obsoletes', multi = True)
        write_var(env, f, 'BuildRequires', 'rpm_build_requires', multi = True)
        write_var(env, f, 'Requires(pre)', 'rpm_pre_requires', multi = True)
        write_var(env, f, 'Requires', 'rpm_requires', multi = True)
        write_var(env, f, 'Requires(postun)', 'rpm_postun_requires',
                  multi = True)

        # Description
        write_spec_text_section(f, env, 'description', 'description')

        # Scripts
        for script in ['prep', 'build', 'install', 'clean', 'pre', 'post',
                       'preun', 'postun', 'verifyscript']:
            write_spec_script(f, env, script, 'rpm_' + script)

        # Files
        if 'rpm_filelist' in env:
            f.write('%%files -f %s\n' % env.get('rpm_filelist'))
        else: f.write('%files\n')
        f.write('%defattr(- root root)\n')

        for files in [
            ['documents', '/usr/share/doc/' + name, '%doc', None],
            ['programs', '/usr/bin', '%attr(0775 root root)', 0755],
            ['scripts', '/usr/bin', '%attr(0775 root root)', 0755],
            ['desktop_menu', '/usr/share/applications', None, None],
            ['init_d', '/etc/init.d', '%config %attr(0775 root root)', None],
            ['config', '/etc/' + name, '%config', None],
            ['icons', '/usr/share/pixmaps', None, None],
            ]:
            install_files(f, env, files[0], build_dir, files[1], files[2],
                          files[3])

        # ChangeLog
        write_spec_text_section(f, env, 'changelog', 'rpm_changelog')

    finally:
        if f is not None: f.close()

    # Create directories needed by rpmbuild
    for dir in ['BUILD', 'BUILDROOT', 'RPMS', 'SOURCES', 'SPECS', 'SRPMS']:
        dir = 'build/' + dir
        if not os.path.exists(dir): os.makedirs(dir)


    # Build the package
    build_dir = os.path.realpath(build_dir)
    cmd = 'rpmbuild -bb --buildroot %s --define "_topdir %s/build" ' \
        '--target %s %s' % (
        build_dir, os.getcwd(), env.GetPackageArch(), spec_file)
    CommandAction(cmd).execute(target, [build_dir], env)

    # Move the package
    target = str(target[0])
    path = 'build/RPMS/' + env.GetPackageArch() + '/' + target
    shutil.move(path, target)


def configure(conf):
    bld = Builder(action = build_function,
                  source_factory = SCons.Node.FS.Entry,
                  source_scanner = SCons.Defaults.DirScanner)
    conf.env.Append(BUILDERS = {'RPM' : bld})

    return True
