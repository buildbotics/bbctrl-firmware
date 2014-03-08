import os
import re
import shutil
import gzip
import bz2

from SCons.Script import *
from SCons.Action import CommandAction


def install_files(env, key, target, perms = 0644, dperms = 0755):
    if key in env: env.CopyToPackage(env.get(key), target, perms, dperms)


def run_command(env, cmd):
    print '@', cmd
    CommandAction(cmd).execute(None, [], env)


def build_function(target, source, env):
    target = str(target[0])
    name = env.get('package_name')

    # Create package build dir
    build_dir = 'build/%s-DMG' % name
    if os.path.exists(build_dir): shutil.rmtree(build_dir)
    os.makedirs(build_dir, 0755)

    # Copy template and possibly decompress to target
    template = env.get('osx_template')
    input = None
    output = None
    try:
        if template.endswith('.gz'): input = gzip.GzipFile(template, 'r')
        elif template.endswith('.bz2'): input = bz2.BZ2File(template, 'r')
        else: input = open(template, 'r')

        output = open(target, 'w')
        
        while True:
            buffer = input.read(1024 * 1024)
            if len(buffer) == 0: break
            output.write(buffer)

    finally:
        if input is not None: input.close()
        if output is not None: output.close()

    # Mount dmg
    run_command(env, '$HDIUTILCOM attach -mountpoint "%s" "%s"' %
                (build_dir, target))

    # Copy files into package
    app_dir = '%s/%s.app' % (build_dir, env.get('osx_app_name'))
    install_files(env, 'documents', app_dir + '/Contents/Resources')
    install_files(env, 'programs', app_dir + '/Contents/MacOS', 0755)
    install_files(env, 'scripts', app_dir + '/Contents/MacOS', 0755)
    install_files(env, 'config', app_dir + '/Contents/Resources')
    install_files(env, 'icons', app_dir + '/Contents/Resources')
    install_files(env, 'osx_files', app_dir, 0755)

    # Unmount dmg
    run_command(env, '$HDIUTILCOM detach "%s"' % build_dir)

    # Compress dmg
    temp = '%s.tmp.dmg' % os.path.splitext(target)[0]
    if os.path.exists(temp): os.unlink(temp)
    shutil.move(target, temp)
    run_command(env, '$HDIUTILCOM convert "%s" -format UDBZ -imagekey '
                'zlib-level=9 -o "%s"' % (temp, target))
    os.unlink(temp)

    # Add SLA
    if 'osx_sla' in env:
        # Unflatten
        run_command(env, '$HDIUTILCOM unflatten "%s"' % target)

        # Add resource
        run_command(env, '$OSXREZCOM /Developer/Headers/FlatCarbon/*.r "%s" -a '
                    '-o "%s"' % (env.get('osx_sla'), target))

        # Flatten
        run_command(env, '$HDIUTILCOM flatten "%s"' % target)

    # Enable Internet access
    run_command(env, '$HDIUTILCOM internet-enable -yes "%s"' % target)


def configure(conf):
    env = conf.env

    env['HDIUTILCOM'] = 'hdiutil'
    env['OSXREZCOM'] = '/Developer/Tools/Rez'

    bld = Builder(action = build_function,
                  source_factory = SCons.Node.FS.Entry,
                  source_scanner = SCons.Defaults.DirScanner)
    env.Append(BUILDERS = {'DMG' : bld})

    return True
