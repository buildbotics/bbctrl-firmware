import textwrap
import re
from platform import *
from SCons.Script import *

ns = ''
ver = None


def escstr(s):
    return s.replace('\\', '\\\\').replace('"', '\\"')


def build_function(target, source, env):
    revision = 'Unknown'
    branch = 'Unknown'
    for line in popen('svn info').readlines():
        if line.startswith('Revision: '): revision = line[10:].strip()
        elif line.startswith('URL: '):
            branch = line[5:].strip()
            branch = re.sub(r'https?://[\w\.]+/(svn/)?', '', branch)

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

    if ns:
        for namespace in ns.split('::'):
            f.write('namespace %s {\n' % namespace)

    if env.get('debug', False): mode = 'Debug'
    else: mode = 'Release'

    f.write(
        '  void addBuildInfo(const char *category) {\n'
        '    Info &info = Info::instance();\n'
        '\n'
        '    info.add(category, "Version", "%s");\n'
        '    info.add(category, "Date", __DATE__);\n'
        '    info.add(category, "Time", __TIME__);\n'
        '    info.add(category, "SVN Rev", "%s");\n'
        '    info.add(category, "Branch", "%s");\n'
        '    info.add(category, "Compiler", COMPILER);\n'
        '    info.add(category, "Options", "%s");\n'
        #'    info.add(category, "Defines", "%s");\n'
        '    info.add(category, "Platform", "%s");\n'
        '    info.add(category, "Bits", String(COMPILER_BITS));\n'
        '    info.add(category, "Mode", "%s");\n'
        '  }\n' % (
            ver, revision, branch,
            escstr(' '.join(env['CXXFLAGS'] + env['CCFLAGS'])),
            #escstr(' '.join(env['CPPDEFINES'])),
            sys.platform.lower() + ' ' + release(), mode,
            ))

    if ns:
        for namespace in ns.split('::'):
            f.write('} // namespace %s\n' % namespace)

    f.close()

    return None


def configure(conf, namespace, version = None):
    global ns, ver

    ns = namespace
    ver = version

    env = conf.env

    bld = Builder(action = build_function,
                  source_factory = SCons.Node.FS.Entry,
                  source_scanner = SCons.Defaults.DirScanner)
    env.Append(BUILDERS = {'BuildInfo' : bld})

    return True
