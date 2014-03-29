import textwrap
import re
from platform import release
import subprocess
from SCons.Script import *


def escstr(s):
    return s.replace('\\', '\\\\').replace('"', '\\"')


def which(program):
    import os
    def is_exe(fpath):
        return os.path.isfile(fpath) and os.access(fpath, os.X_OK)

    fpath, fname = os.path.split(program)
    if fpath:
        if is_exe(program):
            return program
    else:
        for path in os.environ["PATH"].split(os.pathsep):
            path = path.strip('"')
            exe_file = os.path.join(path, program)
            if is_exe(exe_file):
                return exe_file

    return None


def svn_get_info():
    svn = which('svn')
    if not svn: return None, None

    revision, branch = None, None

    try:
        p = subprocess.Popen(['svn', 'info'], stderr = subprocess.PIPE,
                             stdout = subprocess.PIPE)
        out, err = p.communicate()

        for line in out.splitlines():
            if line.startswith('Revision: '): revision = line[10:].strip()
            elif line.startswith('URL: '):
                branch = line[5:].strip()
                branch = re.sub(r'https?://[\w\.]+/(svn/)?', '', branch)

    except Exception, e:
        print e

    return revision, branch


def git_get_info():
    revision, branch = None, None

    try:
        p = subprocess.Popen(['git', 'rev-parse', 'HEAD'],
                             stderr = subprocess.PIPE, stdout = subprocess.PIPE)
        out, err = p.communicate()
        revision = out.strip()

    except Exception, e:
        print e

    try:
        p = subprocess.Popen(['git', 'rev-parse', '--abbrev-ref', 'HEAD'],
                             stderr = subprocess.PIPE, stdout = subprocess.PIPE)
        out, err = p.communicate()
        branch = out.strip()

    except Exception, e:
        print e

    return revision, branch


def build_function(target, source, env):
    contents = (
        '  void addBuildInfo(const char *category) {\n'
        '    Info &info = Info::instance();\n'
        '\n'
        '    info.add(category, "Version", "$PACKAGE_VERSION");\n'
        '    info.add(category, "Date", __DATE__);\n'
        '    info.add(category, "Time", __TIME__);\n')

    repo = 'SVN'
    revision, branch = svn_get_info()
    if revision is None:
        repo = 'Git'
        revision, branch = git_get_info()

    if revision is not None:
        contents += (
            '    info.add(category, "Repository", "%s");\n'
            '    info.add(category, "Revision", "%s");\n'
            '    info.add(category, "Branch", "%s");\n') % (
            repo, revision, branch)

    if env.get('debug', False): mode = 'Debug'
    else: mode = 'Release'

    contents += (
        '    info.add(category, "Compiler", COMPILER);\n'
        '    info.add(category, "Options", "%s");\n'
        '    info.add(category, "Platform", "%s");\n'
        '    info.add(category, "Bits", String(COMPILER_BITS));\n'
        '    info.add(category, "Mode", "%s");\n'
        '  }') % (
            escstr(' '.join(env['CXXFLAGS'] + env['CCFLAGS'])),
            sys.platform.lower() + ' ' + release(), mode,
            )

    target = str(target[0])
    f = open(target, 'w')

    note = ('WARNING: This file was auto generated.  Please do NOT '
            'edit directly or check in to source control.')

    f.write(
        '/' + ('*' * 75) + '\\\n   ' +
        '\n   '.join(textwrap.wrap(note)) + '\n' +
        '\\' + ('*' * 75) + '/\n'
        '\n'
        '#include <cbang/Info.h>\n'
        '#include <cbang/String.h>\n'
        '#include <cbang/util/CompilerInfo.h>\n\n'
        'using namespace cb;\n\n'
        )

    ns = env.subst('$BUILD_INFO_NS')
    if ns:
        for namespace in ns.split('::'):
            f.write(env.subst('namespace %s {\n' % namespace))

    f.write(env.subst(contents) + '\n')

    if ns:
        parts = ns.split('::')
        parts.reverse()
        for namespace in parts:
            f.write('} // namespace %s\n' % namespace)

    f.close()

    return None


def generate(env):
    env.SetDefault(BUILD_INFO_NS = 'BuildInfo')

    bld = Builder(action = build_function,
                  source_factory = SCons.Node.FS.Entry,
                  source_scanner = SCons.Defaults.DirScanner)
    env.Append(BUILDERS = {'BuildInfo' : bld})

    return True


def exists(env):
    return 1
