'''
Builds an OSX application bundle
'''

import os
import shutil
import shlex

from SCons.Script import *


def build_function(target, source, env):
    # Create app dir
    app_dir = env.get('package_name') + '.app'
    if os.path.exists(app_dir): shutil.rmtree(app_dir)

    # Make application dirs
    contents_dir = os.path.join(app_dir, 'Contents')
    resources_dir = os.path.join(contents_dir, 'Resources')
    macos_dir = os.path.join(contents_dir, 'MacOS')
    for d in (contents_dir, resources_dir, macos_dir): os.makedirs(d, 0775)

    # Create Info.plist
    import plistlib
    plist_template = env.get('app_plist_template', None)
    if plist_template is not None: info = plistlib.readPlist(plist_template)
    else: info = {}

    # Info.plist keys
    keys = {
        'CFBundleName': env.get('package_name'),
        'CFBundleVersion': env.get('version'),
        'CFBundleShortVersionString': env.get('version'),
        'CFBundleDisplayName': env.get('package_name'),
        'CFBundleExecutable': env.get('programs', [None])[0],
        'CFBundleIconFile': env.get('icons', [None][0]),
        'CFBundleIdentifier': env.get('app_id'),
        'CFBundlePackageType': env.get('app_type', 'APPL'),
        'CFBundleSignature': env.get('app_signature', None),
        'NSHumanReadableCopyright': env.get('copyright', None),
        }
    for key, value in keys.items():
        if value is not None: info[key] = value

    other = env.get('app_other_info', None)
    if other is not None: info.update(other)

    info_file = os.path.join(contents_dir, 'Info.plist')
    plistlib.writePlist(info, info_file)

    # Let 'app_programs' override 'programs'
    if 'app_programs' in env: programs_key = 'app_programs'
    else: programs_key = 'programs'

    # Copy files into package
    env.InstallFiles('documents', contents_dir + '/SharedSupport')
    env.InstallFiles('config', contents_dir + '/Resources')
    env.InstallFiles('icons', contents_dir + '/Resources')
    env.InstallFiles('changelog', contents_dir + '/SharedSupport')
    env.InstallFiles('license', contents_dir + '/SharedSupport')
    env.InstallFiles(programs_key, contents_dir + '/MacOS', 0755)
    env.InstallFiles('scripts', contents_dir + '/MacOS', 0755)
    env.InstallFiles('app_resources', contents_dir + '/Resources')
    env.InstallFiles('app_shared', contents_dir + '/SharedSupport')
    env.InstallFiles('app_frameworks', contents_dir + '/Frameworks')
    env.InstallFiles('app_plugins', contents_dir + '/PlugIns')

    # Finish command
    finish_cmd = env.get('app_finish_cmd', None)
    if finish_cmd:
        if isinstance(finish_cmd, str): finish_cmd = shlex.split(finish_cmd)
        env.RunCommand(finish_cmd + [app_dir])

    # Zip results
    env.ZipDir(str(target[0]), app_dir)


def generate(env):
    bld = Builder(action = build_function,
                  source_factory = SCons.Node.FS.Entry,
                  source_scanner = SCons.Defaults.DirScanner)
    env.Append(BUILDERS = {'App' : bld})

    return True


def exists():
    return 1

