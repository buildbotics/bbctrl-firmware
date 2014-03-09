'''
Builds an OSX distribution package
'''

from __future__ import absolute_import

import os
import shutil
import plistlib
from xml.etree.ElementTree import ElementTree, Element, SubElement

from SCons.Script import *


def build_function(target, source, env):
    target = str(target[0])

    # Create package build dir
    if target[-4:] != '.zip': raise Exception, 'Expected .zip in package name'
    build_dir = target[:-4] # Remove .zip
    if os.path.exists(build_dir): shutil.rmtree(build_dir)

    # Make dirs
    contents_dir = os.path.join(build_dir, 'Contents')
    packages_dir = os.path.join(contents_dir, 'Packages')
    resources_dir = os.path.join(contents_dir, 'Resources')
    for d in (contents_dir, packages_dir, resources_dir): os.makedirs(d, 0775)

    # Load package info
    pkg_info = []
    packages = env.get('distpkg_packages', [])
    for p in packages:
        infofile = os.path.join(p, 'Contents/Info.plist')
        if os.path.exists(infofile):
            pkg_info.append(plistlib.readPlist(infofile))
        else: raise Exception, 'Missing %s' % infofile

    # Create distribution.dist
    root = Element('installer-script', {'minSpecVersion': '1.0'})

    SubElement(root, 'options', {
            'rootVolumeOnly': 'false' if env.get('distpkg_root_volume_only',
                                                 False) else 'true',
            'allow-external-scripts':
                'yes' if env.get('distpkg_allow_external_scripts',
                                 False) else 'no',
            'customize': env.get('distpkg_customize', 'allow'),
            })

    SubElement(root, 'title').text = env.get('summary')
    for key in 'conclusion license readme welcome'.split():
        if 'distpkg_' + key in env:
            SubElement(root, key, {'file': env.get('distpkg_' + key)})

    background = env.get('distpkg_background', None)
    if background:
        SubElement(root, 'background', {
            'file': background,
            'alignment': 'bottomleft',
            'scaling': 'none',
            })

    pkg_target = env.get('distpkg_target', '10.5')
    pkg_arch = env.get('distpkg_arch', None)
    SubElement(root, 'installation-check', {'script': 'pm_install_check();'})
    SubElement(root, 'volume-check', {'script': 'pm_volume_check();'})
    script_text = """
function pm_volume_check() {
  if(!(my.target.systemVersion &&
    system.compareVersions(my.target.systemVersion.ProductVersion, '""" + \
    pkg_target + """') >= 0)) {
    my.result.title = 'Unable to Install';
    my.result.message = 'OSX """ + pkg_target + """ or later is required.';
    my.result.type = 'Fatal';
    return false;
  }
  return true;
}
function pm_install_check() {
  if((system.sysctl('hw.cputype') == '18')) {
    my.result.title = 'Unable to Install';
    my.result.message = 'An intel-based Mac is required.';
    my.result.type = 'Fatal';
    return false;
  }"""
    if pkg_arch == 'x86_64':
        script_text += """
  if(system.sysctl('hw.optional.x86_64') == false) {
    my.result.title = 'Unable to Install';
    my.result.message = 'A 64-bit cpu is required (core 2 duo or better).';
    my.result.type = 'Fatal';
    return false;
  }"""
    script_text += """
  return true;
}
"""
    SubElement(root, 'script').text = script_text

    outline = SubElement(root, 'choices-outline')
    for i in range(len(pkg_info)):
        info = pkg_info[i]
        id = info['CFBundleIdentifier']
        choice_id = 'choice' + str(i)
        choice_title = os.path.basename(packages[i])
        SubElement(outline, 'line', {'choice': choice_id})

        choice = SubElement(root, 'choice', {
                'id': choice_id,
                'title': choice_title,
                'description': '',
                })

        SubElement(choice, 'pkg-ref', {'id': id})

        pkg_path = 'file:./Contents/Packages/' + os.path.basename(packages[i])
        pkg_ref_info =  {
                'id': id,
                'installKBytes': str(info['IFPkgFlagInstalledSize']),
                'version': info['CFBundleShortVersionString'],
                }
        try:
            if info['IFPkgFlagAuthorizationAction'] == 'RootAuthorization':
                pkg_ref_info['auth'] = 'Root'
        except: pass
        SubElement(root, 'pkg-ref', pkg_ref_info).text = pkg_path

    tree = ElementTree(root)
    f = None
    try:
        f = open(contents_dir + '/distribution.dist', 'w')
        tree.write(f, 'utf-8')
    finally:
        if f is not None: f.close()

    # Install files
    env.InstallFiles('distpkg_resources', resources_dir)
    env.InstallFiles('distpkg_packages', packages_dir, perms = 0)

    # Zip results
    env.ZipDir(target, build_dir)


def generate(env):
    bld = Builder(action = build_function,
                  source_factory = SCons.Node.FS.Entry,
                  source_scanner = SCons.Defaults.DirScanner)
    env.Append(BUILDERS = {'DistPkg' : bld})

    return True


def exists():
    return 1

