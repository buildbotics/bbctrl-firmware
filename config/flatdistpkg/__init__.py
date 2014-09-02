# flatdistpkg.py

import os, platform, shutil, subprocess
import config
import zipfile

from SCons.Script import *
from SCons.Action import CommandAction

try:
    have_lxml = False
    from lxml import etree
    have_lxml = True
except:
    import xml.etree.ElementTree as etree

import glob
#import plistlib
import json
from pprint import pprint


filename_package_build_txt = 'package.txt'
filename_package_desc_txt = 'package-description.txt'
filename_package_info_json = 'package-parameters.json'
filename_distribution_xml = 'distribution.xml'

build_dir = 'build'
build_dir_tmp = build_dir + '/distpkg/tmp'
build_dir_resources = build_dir + '/distpkg/Resources'
build_dir_packages = build_dir + '/distpkg/Packages'
build_dir_stage = build_dir + '/flatpkg' # appended with /package_name

build_dirs = [
    build_dir_tmp, build_dir_resources,
    build_dir_packages, build_dir_stage,
    ]

# probably don't need these; info should be passed via FlatDistPackager()
default_src_resources = 'Resources'
# going to generate this from scratch, so won't need template
default_src_distribution_xml = 'src/distribution.xml'

build_dir_distribution_xml = \
    os.path.join(build_dir_tmp, filename_distribution_xml)


def RunCommandOrRaise(env, cmd):
    print '@', cmd
    ret = CommandAction(cmd).execute(None, [], env)
    if ret: raise Exception, 'command failed, return code %s' % str(ret)


def clean_old_build(env):
    # rm intermediate build stuff
    # do not rm old build products, descriptions
    if os.path.exists(build_dir):
        shutil.rmtree(build_dir)


def setup_dirs(env):
    return
    # FUTURE USE
    # build/distpkg/package_name/{tmp,Resources,Packages}
    # build/flatpkg/package_name/{root,Resources,Scripts}
    # useful if a project creates multiple distpkg
    # clean_old_build would need to only rm build/distpkg/thisname
    # would also want to separate flatpkg building
    name = env['package_name']
    env['build_dir'] = 'build'
    env['build_dir_tmp'] = tmp = \
        build_dir + '/distpkg/' + name + '/tmp'
    env['build_dir_resources'] = res = \
        build_dir + '/distpkg/' + name + '/Resources'
    env['build_dir_packages'] = packs = \
        build_dir + '/distpkg/' + name + '/Packages'
    env['build_dir_stage'] = stage = build_dir + '/flatpkg'
    env['build_dirs'] = [tmp,res,packs,stage]
    env['build_dir_distribution_xml'] = \
        os.path.join(tmp, filename_distribution_xml)


def create_dirs(env):
    #setup_dirs(env)
    dirs = build_dirs #+ env.get('build_dirs',[])
    for d in dirs:
        if not os.path.isdir(d):
            os.makedirs(d, 0755)
    #env.Dir(build_dir)
    # above fails with
    # scons: *** [fah-installer_7.2.12_intel.pkg.zip] TypeError :
    #   Tried to lookup File 'build' as a Dir.


def remove_cruft_from_directory(d, env):
    if not (d and os.path.isdir(d) and d.startswith('build/')):
        return
    # rm any lingering cruft (.svn .DS_Store)
    # other candidates may include
    #    ._??*, resource forks, ACLs, xattrs
    # In SConstruct, should use env['package_ignores'] += ['.DS_Store']
    # FIXME NOT FULLY IMPLEMENTED
    # FIXME should probably use os.walk()
    cmd = ['find',d,'-type','f','-name','.DS_Store','-print','-delete']
    env.RunCommand(cmd)
    return
    # feels too dangerous
    cmd = ['find', d, '-type', 'd', '-name', '.svn', '-exec',
        'rm', '-rf', '{}', ';', '-prune', '-print']
    env.RunCommand(cmd)
    # UNTESTED:
    cruft_files = ['.DS_Store']
    cruft_dirs = ['.svn']
    for root, dirs, files in os.walk(d):
        for name in cruft_files:
            if name in files:
                path = os.path.join(root, name)
                print 'WOULD BE deleting ' + path
                #os.remove(path)
        for name in cruft_dirs:
            if name in dirs:
                dirs.remove(name)
                path = os.path.join(root, name)
                print 'WOULD BE deleting directory ' + path
                #shutil.rmtree(path)


def rename_prepostflight_scripts(scripts_dir):
    # unless PackageInfo says otherwise,
    # flat pkgs only use pre/postinstall
    pre1 = os.path.join(scripts_dir, 'preflight')
    pre2 = os.path.join(scripts_dir, 'preinstall')
    post1 = os.path.join(scripts_dir, 'postflight')
    post2 = os.path.join(scripts_dir, 'postinstall')
    if os.path.isfile(pre1) and not os.path.exists(pre2):
        print 'renaming %s to %s' % (pre1, pre2)
        os.rename(pre1, pre2)
    if os.path.isfile(post1) and not os.path.exists(post2):
        print 'renaming %s to %s' % (post1, post2)
        os.rename(post1, post2)


def build_component_pkg(info, env):
    # FIXME -- possibly incomplete and makes assumptions
    # build component from info using pkgbuild
    # uses globals build_dir_stage, build_dir_packages
    name = info.get('name')
    home = info.get('home')
    pkg_id = info.get('pkg_id')
    # if no comp version, use distpkg version
    version = info.get('version', env.get('version'))
    root = info.get('root')
    resources = info.get('resources')
    pkg_resources = info.get('pkg_resources')
    install_to = info.get('install_to', '/')
    pkg_nopayload = info.get('pkg_nopayload', False)

    if not name:
        raise Exception, 'component has no name'
    if not home:
        raise Exception, 'component %s has no home' % name
    if not pkg_id:
        raise Exception, 'component %s has no pkg_id' % name
    if not version:
        raise Exception, 'no version for component %s' % name

    stage = os.path.join(build_dir_stage, name)
    stage_resources = os.path.join(stage, 'Resources')

    target = os.path.join(build_dir_packages, name + '.pkg')

    # FIXME uses knowledge of how pkg.py works
    if not root:
        root = home + '/build/pkg/root'
    if not resources:
        resources = home + '/build/pkg/Resources'

    # FIXME uses knowledge of layout of most projects
    if pkg_resources is None:
        d = os.path.join(home,'osx/Resources')
        if d and os.path.isdir(d):
            pkg_resources = [[d, '.']]

    # FIXME uses knowledge of how pkg_scripts has been used
    # doesn't handle pkg_scripts as tuple list, like pkg_resources always is
    scripts = os.path.join(home, info.get('pkg_scripts', 'osx/scripts'))
    stage_scripts = None
    if scripts:
        scripts_dir_name = os.path.basename(scripts)
        stage_scripts = os.path.join(stage, scripts_dir_name)

    # make needed dirs
    if not os.path.isdir(stage): os.makedirs(stage)

    # if root doesn't exist, do full build of root, same as pkg.py does
    # need package-parameters.json to do so
    # TODO
    if not os.path.isdir(root):
        pass

    if not os.path.isdir(root) and not pkg_nopayload:
        raise Exception, '%s component root does not exist! %s' % (name,root)

    # try to copy scripts to our stage and use that to avoid most cruft
    # also allows renaming scripts before pkgbuild
    if scripts and os.path.isdir(scripts):
        env.CopyToPackage(scripts, stage, perms=0755)

    # copy resources to our stage for future distpkg use
    if resources and os.path.isdir(resources):
        env.CopyToPackage(resources, stage)
    elif pkg_resources:
        env.CopyToPackage(pkg_resources, stage_resources)

    # not needed, done later to expanded distpkg anyway
    #remove_cruft_from_directory(stage, env)

    rename_prepostflight_scripts(stage_scripts)

    # if any apps/tools should be codesign'd do that now
    # assumes project didn't do it when creating its distroot
    # TODO clone env and add info so all sign_ vars can be overridden
    sign_apps = info.get('sign_apps', [])
    for path in sign_apps:
        if not path.startswith('/'):
            path = os.path.join(root, path)
        sign_application(path, env)

    sign_tools = info.get('sign_tools', [])
    for path in sign_tools:
        if not path.startswith('/'):
            path = os.path.join(root, path)
        sign_executable(path, env)

    cmd = ['pkgbuild',
        '--install-location', install_to,
        '--version', version,
        '--identifier', pkg_id,
        ]

    if pkg_nopayload:
        cmd += ['--nopayload']
    else:
        cmd += ['--root', root]

    if stage_scripts and os.path.isdir(stage_scripts):
        cmd += ['--scripts', stage_scripts]
    elif scripts and os.path.isdir(scripts):
        cmd += ['--scripts', scripts]
    cmd += [target]
    RunCommandOrRaise(env, cmd)


def build_component_pkgs(env):
    components = env.get('distpkg_components', None)

    if not components:
        # in future, maybe handle as implicit pkg+distpkg if pkg_type dist
        raise Exception, 'no components specified'

    # validate and fill-in any missing info
    for info in components:
        # home is required
        home = info.get('home')
        if not home:
            name = info.get('name') # might be None at this point
            raise Exception, 'home not provided for component ' + name
        # name and pkg_id are also required, but may be in json

        # try to load pkg info json, if exists
        # merge with components info
        # FIXME UNTESTED AND MAYBE INCOMPLETE
        if not info.get('params_json_loaded'):
            jsonfile = os.path.join(home, filename_package_info_json)
            if os.path.isfile(jsonfile):
                print 'loading info from %s' % jsonfile
                info2 = json.load(jsonfile)
                # merge
                # trust what we were passed over what json says
                # this allows distpkg to override component pkg info
                info2.update(info)
                info = info2
                info['params_json_loaded'] = True

        name = info.get('name')
        if not info.get('package_name'):
            info['package_name'] = name
        if not info.get('package_name_lower'):
            info['package_name_lower'] = name.lower()

        # try to get description, if still none
        desc = info.get('short_description', '').strip()
        if not desc:
            desc = info.get('description', '').strip()
        if not desc:
            fname = os.path.join(home, filename_package_desc_txt)
            if os.path.isfile(fname):
                f = None
                try:
                    f = open(fname, 'r')
                    desc = f.read().strip()
                finally:
                    if f is not None: f.close()
        if not desc:
            desc = info.get('summary', '').strip()
        if desc:
            desc = desc.replace('\n',' ')
            desc = desc.replace('  ',' ').replace('  ',' ')
            desc = desc.replace('. ','.  ')
        if desc:
            info['short_description'] = desc

    for info in components:
        try:
            build_component_pkg(info, env)
        except Exception, e:
            print 'unable to build component ' + info.get('name')
            raise e

    # replace distpkg_components with modified/created info list
    env['distpkg_components'] = components


def build_product_pkg(target, source, env):
    version = env.get('version')
    pkg_id = env.get('distpkg_id')
    cmd = ['productbuild',
        '--distribution', build_dir_distribution_xml,
        '--package-path', build_dir_packages,
        '--resources', build_dir_resources,
        '--version', version,
        ]
    if pkg_id: cmd += ['--identifier', pkg_id]
    cmd += [target]
    RunCommandOrRaise(env, cmd)


def expand_flat_pkg(target, source, env):
    cmd = ['pkgutil', '--expand', source, target]
    RunCommandOrRaise(env, cmd)


def flatten_to_pkg(target, source, env):
    cmd = ['pkgutil', '--flatten', source, target]
    RunCommandOrRaise(env, cmd)


def unlock_keychain(env, keychain=None, password=None):
    if keychain is None: keychain = env.get('sign_keychain', None)
    if keychain: name = keychain
    else: name = 'default-keychain'
    if password is None:
        passfile = os.path.expanduser('~/.ssh/p')
        if os.path.isfile(passfile):
            f = None
            try:
                f = open(passfile, 'r')
                password = f.read().strip('\n')
            finally:
                if f is not None: f.close()
    if password:
        cmd = ['security', 'unlock-keychain', '-p', password]
        if keychain: cmd += [keychain]
        try:
            sanitized_cmd = cmd[:3] + ['xxxxxx']
            if keychain: sanitized_cmd += [keychain]
            print '@', sanitized_cmd
            # returns 0 if keychain already unlocked, even if pass is wrong
            ret = CommandAction(cmd).execute(None, [], env)
            if ret: raise Exception, \
                'unlock-keychain failed, return code %s' % str(ret)
        except Exception, e:
            print 'unable to unlock keychain "%s"' % name
            raise e
    else:
        print 'skipping unlock "%s"' % name + '; no password given'


def sign_flat_package(target, source, env):
    # expect source 'somewhere/tmpname.pkg', target elsewhere/finalname.pkg
    sign = env.get('sign_id_installer', None)
    keychain = env.get('sign_keychain', None)
    if not sign:
        raise Exception, 'unable to sign; no sign_id_installer provided'
    # FIXME should do more than this
    x, ext = os.path.splitext(source)
    if not (os.path.isfile(source) and ext in ('.pkg', '.mpkg')):
        raise Exception, 'unable to sign; not a flat package: ' + source
    cmd = ['productsign', '--sign', sign]
    if keychain:
        cmd += ['--keychain', keychain]
    cmd += [source, target]
    RunCommandOrRaise(env, cmd)


def sign_or_copy_product_pkg(target, source, env):
    if env.get('sign_id_installer'):
        sign_flat_package(target, source, env)
        return
    print 'NOT signing package; no sign_id_installer provided; copying instead...'
    shutil.copy2(source, target)


def sign_application(target, env):
    keychain = env.get('sign_keychain')
    sign = env.get('sign_id_app')
    cmd = ['codesign', '-f']
    try:
      ver = tuple([int(x) for x in platform.mac_ver()[0].split('.')])
      # all bundles in an app must also be signed on 10.7+
      # easy way to do this is just to codesign --deep
      # only ok if there are no sandbox entitlements for this app
      if ver >= (10,7): cmd += ['--deep']
    except: pass
    if keychain:
        cmd += ['--keychain', keychain]
    if sign:
        cmd += ['--sign', sign]
    else:
        raise Exception, 'unable to codesign %s; no sign_id_app given' % target
    if not os.path.isdir(target) or not target.endswith('.app'):
        raise Exception, 'unable to codesign %s; not an app' % target
    cmd += [target]
    RunCommandOrRaise(env, cmd)


def sign_executable(target, env):
    keychain = env.get('sign_keychain')
    sign = env.get('sign_id_app')
    prefix = env.get('sign_prefix')
    cmd = ['codesign', '-f']
    if keychain:
        cmd += ['--keychain', keychain]
    if prefix:
        if not prefix.endswith('.'): prefix += '.'
        cmd += ['--prefix', prefix]
    else:
        raise Exception, 'unable to codesign %s; no sign_prefix given' % target
    if sign:
        cmd += ['--sign', sign]
    else:
        raise Exception, 'unable to codesign %s; no sign_id_app given' % target
    if not (os.path.isfile(target) and os.access(target, os.X_OK)):
        raise Exception, 'unable to codesign %s; not an executable' % target
    # FIXME should not try to sign executable scripts
    cmd += [target]
    RunCommandOrRaise(env, cmd)


def build_or_copy_distribution_template(env):
    target = build_dir_distribution_xml
    source = default_src_distribution_xml

    build_distribution_template(env, target)

    if os.path.exists(target): return

    print 'WARNING: did not generate distribution.xml; ' \
        + 'using pre-built template %s' % source
    if not os.path.isfile(source):
        raise Exception, 'pre-built template does not exist %s' % source
    #Execute(Copy(target, source))
    # sometimes after a 'scons --clean', above fails with
    # scons: *** [fah-installer_7.2.12_intel.pkg.zip] TypeError :
    #   Tried to lookup File 'build' as a Dir.
    shutil.copy2(source, target)


def build_distribution_template(env, target=None):
    if not target:
        target = build_dir_distribution_xml

    print 'generating ' + target

    distpkg_target = env.get('distpkg_target', '10.5')
    distpkg_arch = env.get('distpkg_arch')
    if not distpkg_arch:
        distpkg_arch = env.get('package_arch')
    if distpkg_arch in (None, '', 'intel'):
        print 'using distpkg_arch i386 instead of "%s"' % distpkg_arch
        distpkg_arch = 'i386'

    # generate new tree via lxml
    root = etree.Element('installer-script', {'minSpecVersion':'1'})
    tree = etree.ElementTree(root)

    etree.SubElement(root, 'title').text = \
        env.get('package_name_lower') + '_title'

    # non-root pkg installs are very buggy, so this should stay True
    distpkg_root_volume_only = env.get('distpkg_root_volume_only', True)
    allow_external = env.get('distpkg_allow_external_scripts', False)
    if allow_external: allow_external = 'yes'
    else: allow_external = 'no'
    allow_ppc = distpkg_arch and 'ppc' in distpkg_arch and \
        (distpkg_target.split('.') < '10.6'.split('.'))
    if allow_ppc: hostArchitectures = 'i386,ppc'
    else: hostArchitectures = 'i386'
    if distpkg_root_volume_only: rootVolumeOnly = 'true'
    else: rootVolumeOnly = 'false'
    opts = {
        'allow-external-scripts': allow_external,
        'customize': env.get('distpkg_customize', 'allow'),
        # i386 covers both 32 and 64 bit kernel, NOT cpu
        # cpu 64 bit check must be done in script
        'hostArchitectures': hostArchitectures,
        'rootVolumeOnly': rootVolumeOnly,
        }
    etree.SubElement(root, 'options', opts)

    # WARNING domains element is buggy for anything other then root-only.
    # It also leads to support questions, because default selection must be
    # clicked before can click continue. And it gives an uninformative,
    # unhelpful message if volume check fails.
    # So, disabled for now. May need to enable in the future if
    # options rootVolumeOnly becomes unsupported.
    if False and distpkg_root_volume_only:
        etree.SubElement(root, 'domains', {
            'enable_anywhere': 'false',
            'enable_currentUserHome': 'false',
            'enable_localSystem': 'true',
            })

    for key in 'welcome license readme conclusion'.split():
        if 'distpkg_' + key in env:
            etree.SubElement(root, key, {'file': env.get('distpkg_' + key)})

    background = env.get('distpkg_background', None)
    if background:
        etree.SubElement(root, 'background', {
            'alignment': 'bottomleft',
            'file': background,
            'scaling': 'none',
            })

    ic = etree.SubElement(root, 'installation-check', {
        'script': 'install_check();'})
    vc = etree.SubElement(root, 'volume-check', {
        'script': 'volume_check();'})

    if distpkg_target:
        e = etree.Element('allowed-os-versions')
        etree.SubElement(e, 'os-version', {'min':distpkg_target})
        vc.append(e)

    # options hostArchitectures is sufficient on 10.5+, but no clear
    # message is given by Installer.app, so we still want potential
    # hw.cputype check in install_check
    script_text = """
function is_min_version(ver) {
  if (my.target && my.target.systemVersion &&
    system.compareVersions(my.target.systemVersion.ProductVersion, ver) >= 0)
    return true;
  return false;
}
function have_64_bit_cpu() {
  if (system.sysctl('hw.optional.x86_64'))
    return true;
  if (system.sysctl('hw.cpu64bit_capable'))
    return true;
  if (system.sysctl('hw.optional.64bitops'))
    return true;
  return false;
}
function volume_check() {
  if (!is_min_version('""" + distpkg_target + """')) {
    my.result.title = 'Unable to Install';
    my.result.message = 'OSX """ + distpkg_target + \
    """ or later is required.';
    my.result.type = 'Fatal';
    return false;
  }
  return true;
}
function install_check() {"""

    if not (distpkg_arch and 'ppc' in distpkg_arch) and \
        (distpkg_target.split('.') < '10.6'.split('.')):
        script_text += """
  if (system.sysctl('hw.cputype') == '18') {
    my.result.title = 'Unable to Install';
    my.result.message = 'An Intel-based Mac is required.';
    my.result.type = 'Fatal';
    return false;
  }"""

    if distpkg_arch == 'x86_64':
        script_text += """
  if (have_64_bit_cpu() == false) {
    my.result.title = 'Unable to Install';
    my.result.message ='A 64-bit processor is required (Core 2 Duo or better).';
    my.result.type = 'Fatal';
    return false;
  }""" 

    script_text += """
  return true;
}
""" # end install_check

    outline = etree.SubElement(root, 'choices-outline')

    components = env.get('distpkg_components')
    pkgrefs = []
    for info in components:
        pkg_id = info.get('pkg_id')
        name_lower = info.get('package_name_lower')
        pkg_target = info.get('distpkg_target', distpkg_target)
        choice_id = name_lower
        # remove any dots or spaces in choice_id for 10.5 compatibility
        choice_id = choice_id.replace('.','').replace(' ','')
        choice_title = info.get('package_name', info.get('summary'))
        # description is in Localizable.strings
        choice_desc = info.get('package_name_lower') + '_desc'
        etree.SubElement(outline, 'line', {'choice':choice_id})
        choice = etree.SubElement(root, 'choice', {
                'id': choice_id,
                'title': choice_title,
                'description': choice_desc,
                })
        etree.SubElement(choice, 'pkg-ref', {'id': pkg_id})
        pkg_path = info.get('package_name') + '.pkg'
        pkg_ref_info =  {
            'id': pkg_id,
            # version and installKBytes will be added by productbuild
            }
        if distpkg_root_volume_only or info.get('pkg_root_volume_only'):
            pkg_ref_info['auth'] = 'Root'
        ref = etree.Element('pkg-ref', pkg_ref_info)
        ref.text = pkg_path
        pkgrefs.append(ref)
        must_close_apps = info.get('must_close_apps', [])
        if must_close_apps:
            r = etree.Element('pkg-ref', {'id':pkg_id})
            must_close = etree.SubElement(r, 'must-close')
            for app_id in must_close_apps:
                etree.SubElement(must_close, 'app', {'id':app_id})
            pkgrefs.append(r)
        # if appropriate, add start_{enabled,selected} attrs and script
        if pkg_target and pkg_target.split('.') > distpkg_target.split('.'):
            # add start_{enabled,selected} attrs to choice
            # add min version scripts to script_text
            # TODO add arch checks if component has diff reqs than distpkg
            choice.set('start_enabled', name_lower + '_start_enabled()')
            choice.set('start_selected', name_lower + '_start_selected()')
            script_text += """
function """ + name_lower + """_start_enabled() {
  return is_min_version('""" + pkg_target + """');
}
function """ + name_lower + """_start_selected() {
  return is_min_version('""" + pkg_target + """');
}
"""
    for ref in pkgrefs: root.append(ref)

    if have_lxml:
        etree.SubElement(root, 'script').text = etree.CDATA(script_text)
    else:
        etree.SubElement(root, 'script').text = script_text

    # save tree to target
    if have_lxml:
        tree.write(target,encoding='utf-8',standalone=True,pretty_print=True)
    else:
        f = None
        try:
            f = open(target, 'w')
            tree.write(f, encoding='utf-8')
        finally:
            if f is not None: f.close()
    return


def patch_expanded_pkg_distribution(target, source, env):
    # fixup whatever needs changing for < 10.6.6 compatibility
    fpath = os.path.join(target, 'Distribution')
    # load xml
    tree = None
    if os.path.isfile(fpath):
        tree = etree.parse(fpath)
    # make changes
    if tree:
        print 'patching ' + fpath
        root = tree.getroot()
        if root.tag != 'installer-script':
            print 'changing root tag from %s to installer-script' % root.tag
            root.tag = 'installer-script'
        minSpecVersion = root.get('minSpecVersion')
        if minSpecVersion != '1':
            print 'changing minSpecVersion from %s to 1' % minSpecVersion
            root.set('minSpecVersion', '1')
        if have_lxml:
            # avoid losing CDATA for <script> text
            script = root.find('script')
            if script is not None:
                text = script.text
                script.text = etree.CDATA(text)
        # overwrite back to fpath
        if have_lxml:
            tree.write(fpath, encoding='utf-8',
                standalone=True, pretty_print=True)
        else:
            f = None
            try:
                f = open(fpath, 'w')
                tree.write(f, encoding='utf-8')
            finally:
                if f is not None: f.close()
        # FIXME detect any failures somehow
        return
    # failed to load xml
    print 'WARNING: unable to load and patch %s' % fpath
    print 'WARNING: pkg may require OSX 10.6.6 or later'


def create_localizable_strings(env):
    print 'generating Localizable.strings'
    # English only for now
    components = env.get('distpkg_components', [])
    if components is None: components = []
    lines = []

    # FUTURE
    # load template files into lines
    # src/Localizable.strings
    # Resources/en.lproj/Localizable.strings
    # ...

    # add distpkg summary aka dist package title
    summary = env.get('summary', env.get('package_name'))
    if summary:
        key = env.get('package_name_lower') + '_title'
        line = '"%s" = "%s";' % (key, summary)
        lines.append(line)
    # really should merge multiple Localizable.strings files, for mult lang
    for comp in components:
        key = comp['name'].lower() + '_desc'
        value = comp.get('short_description', comp.get('description', ''))
        line = '"%s" = "%s";' % (key, value)
        lines.append(line)
        if not value:
            print 'no description found for component ' + comp['name']
    # save it (overwrites if exists)
    d = build_dir_resources + '/en.lproj'
    if not os.path.isdir(d): os.makedirs(d, 0755)
    fname = d + '/Localizable.strings'
    print 'writing ' + fname
    env.WriteStringToFile(fname, lines)


def flat_dist_pkg_build(target, source, env):
    # expect target package_name_version_arch.pkg.zip
    target_zip = str(target[0])
    # validate target, source, env
    # tolerate .mpkg, because installer seems semi-ok with it
    # TODO allow no .zip, and no ZipDir()
    target_pkg, ext = os.path.splitext(target_zip)
    if ext != '.zip': raise Exception, 'Expected .zip in package name'
    target_base, ext = os.path.splitext(target_pkg)
    if not ext in ('.pkg', '.mpkg'):
        raise Exception, 'Expected .pkg or .mpkg in package name'

    # .mpkg causes errors in log, and does not work on 10.5,
    # so use .pkg for flat package, even if final target is .mpkg.zip
    if ext == '.mpkg': target_pkg = target_base + '.pkg'

    # flat packages require OS X 10.5+
    distpkg_target = env.get('distpkg_target', '10.5')
    if distpkg_target.split('.') < '10.5'.split('.'):
        raise Exception, \
            'incompatible configuration: flat package and osx pre-10.5 (%s)' \
            % distpkg_target

    clean_old_build(env)
    create_dirs(env)

    target_pkg_tmp = os.path.join(build_dir_tmp, target_base + '.tmp.pkg')
    target_pkg_expanded = os.path.join(build_dir_tmp, target_base+'.expanded')
    target_unsigned = os.path.join(build_dir_tmp, target_pkg)

    # TODO: more validation here ...

    name = env.get('package_name')
    print 'Building "%s" flat, target %s' % (name, distpkg_target)

    # copy our own dist Resources
    if env.get('distpkg_resources'):
        env.InstallFiles('distpkg_resources', build_dir_resources)
    elif env.get('pkg_resources'):
        env.InstallFiles('pkg_resources', build_dir_resources)

    unlock_keychain(env)

    build_component_pkgs(env)

    # probably not needed
    # I think build_component_pkgs always raises on any failure
    components = env.get('distpkg_components')
    if components is None or not len(components):
        raise Exception, 'No distpkg_components. Cannot continue.'

    # create Localizable.strings with dist title, component descriptions
    # this must be done AFTER components are built
    create_localizable_strings(env)

    build_or_copy_distribution_template(env)

    build_product_pkg(target_pkg_tmp, [], env)

    expand_flat_pkg(target_pkg_expanded, target_pkg_tmp, env)

    patch_expanded_pkg_distribution(target_pkg_expanded, [], env)

    # if built any components using home/osx/scripts, we still need to rename
    for d in glob.glob(os.path.join(target_pkg_expanded, '*.pkg/Scripts')):
        rename_prepostflight_scripts(d)

    remove_cruft_from_directory(target_pkg_expanded, env)

    flatten_to_pkg(target_unsigned, target_pkg_expanded, env)

    sign_or_copy_product_pkg(target_pkg, target_unsigned, env)

    env.ZipDir(target_zip, target_pkg)

    # write package-description.txt
    # these are currently also generated by
    # {client,viewer}/SConstruct and control/setup.py
    # it should be done by pkg.py
    # or projects can drop a package-parameters.json so we can get all info
    desc = env.get('short_description', '').strip()
    if not desc:
        desc = env.get('description', '').strip()
    if not desc:
        desc = env.get('summary', '').strip()
    # note: desc may be '', write anyway
    print 'writing ' + filename_package_desc_txt
    env.WriteStringToFile(filename_package_desc_txt, desc)

    # write package.txt
    print 'writing ' + filename_package_build_txt
    env.WriteStringToFile(filename_package_build_txt, target_zip)


def generate(env):
    bld = Builder(action = flat_dist_pkg_build,
                  source_factory = SCons.Node.FS.Entry,
                  source_scanner = SCons.Defaults.DirScanner)
    env.Append(BUILDERS = {'FlatDistPkg' : bld})
    return True


def exists():
  return 1

