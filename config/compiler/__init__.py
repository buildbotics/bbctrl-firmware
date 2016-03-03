import copy
import re
import os
from platform import machine, architecture
from SCons.Script import *
from subprocess import *
from SCons.Util import MD5signature
import SCons.Action
import SCons.Builder
import SCons.Tool


class decider_hack:
    def __init__(self, env):
        self.env = env
        self.decider = env.decide_source

    def __call__(self, dep, target, prev_ni):
        try:
            csig = dep.csig
        except AttributeError:
            csig = MD5signature(dep.get_contents())
            major, minor = SCons.__version__.split('.')[:2]
            if major < 2 or (major == 2 and minor < 4): dep.csig = csig
            dep.get_ninfo().csig = csig

        #print dependency, csig, "?=", prev_ni.csig
        if prev_ni is None: return True
        try:
            return csig != prev_ni.csig
        except AttributeError:
            return True


def CheckRDynamic(context):
    context.Message('Checking for -rdynamic...')
    env = context.env
    flags = env['LINKFLAGS']
    context.env.AppendUnique(LINKFLAGS = ['-rdynamic'])
    result = context.TryLink('int main(int argc, char *argv[]) {return 0;}',
                             '.c')
    context.Result(result)
    env['LINKFLAGS'] = flags
    return result


def configure(conf, cstd = 'c99'):
    env = conf.env
    if env.GetOption('clean'): return

    # Decider hack.  Works around some SCons bugs.
    env.Decider(decider_hack(env))

    # Get options
    debug = int(env.get('debug'))
    optimize = int(env.get('optimize'))
    if optimize == -1: optimize = not debug
    globalopt = int(env.get('globalopt'))
    sse2 = int(env.get('sse2'))
    sse3 = int(env.get('sse3'))
    auto_dispatch = int(env.get('auto_dispatch'))
    strict = int(env.get('strict'))
    threaded = int(env.get('threaded'))
    profile = int(env.get('profile'))
    depends = int(env.get('depends'))
    compiler = env.get('compiler')
    distcc = int(env.get('distcc'))
    ccache = int(env.get('ccache'))
    ccflags = env.get('ccflags')
    linkflags = env.get('linkflags')
    cxxstd = env.get('cxxstd')
    platform = env.get('platform')
    static = int(env.get('static'))
    num_jobs = env.get('num_jobs')
    osx_min_ver = env.get('osx_min_ver')
    osx_sdk_root = env.get('osx_sdk_root')
    osx_archs = env.get('osx_archs')
    win32_thread = env.get('win32_thread')
    cross_mingw = int(env.get('cross_mingw'))

    if cross_mingw:
        res_action = SCons.Action.Action('$RCCOM', '$RCCOMSTR')
        res_builder = \
            SCons.Builder.Builder(action = res_action, suffix='.o',
                                  source_scanner = SCons.Tool.SourceFileScanner)
        SCons.Tool.SourceFileScanner.add_scanner('.rc', SCons.Defaults.CScan)

        env['RC'] = 'windres'
        env['RCFLAGS'] = SCons.Util.CLVar('')
        env['RCINCFLAGS'] = '$( ${_concat(RCINCPREFIX, CPPPATH, ' +\
            'RCINCSUFFIX, __env__, RDirs, TARGET)} $)'
        env['RCINCPREFIX'] = '--include-dir '
        env['RCINCSUFFIX'] = ''
        env['RCCOM'] = '$RC $RCINCFLAGS $RCINCPREFIX $SOURCE.dir $RCFLAGS ' +\
            '-i $SOURCE -o $TARGET'
        env['BUILDERS']['RES'] = res_builder
        env['PROGSUFFIX'] = '.exe'

        env.CBDefine('WINVER=0x0600')
        env.CBDefine('_WIN32_WINNT=0x0600')
        env.CBDefine('_POSIX_SOURCE')

    if platform != '': env.Replace(PLATFORM = platform)

    # Select compiler
    compiler_mode = None

    # Prefer Intel compiler
    if compiler == 'default' and os.environ.get('INTEL_LICENSE_FILE', False):
        compiler = 'intel'

    if compiler:
        if compiler == 'gnu':
            Tool('gcc')(env)
            Tool('g++')(env)
            compiler_mode = "gnu"

        elif compiler == 'clang':
            env.Replace(CC = 'clang')
            env.Replace(CXX = 'clang++')
            compiler_mode = "gnu"

        elif compiler == 'intel':
            Tool('intelc')(env)
            env['ENV']['INTEL_LICENSE_FILE'] = (
                os.environ.get("INTEL_LICENSE_FILE", ''))

            if env['PLATFORM'] == 'win32': compiler_mode = 'msvc'
            else: compiler_mode = 'gnu'

            if compiler_mode == 'msvc':
                env.Replace(AR = 'xilib')

                # Work around double CCFLAGS bug
                env.Replace(CXXFLAGS = ['/TP'])

                # Fix INCLUDE
                env.PrependENVPath('INCLUDE', os.environ.get('INCLUDE', ''))

        elif compiler == 'linux-mingw':
            env.Replace(CC = 'i586-mingw32msvc-gcc')
            env.Replace(CXX = 'i586-mingw32msvc-g++')
            env.Replace(RANLIB = 'i586-mingw32msvc-ranlib')
            env.Replace(PROGSUFFIX = '.exe')
            compiler_mode = "gnu"

        elif compiler == 'posix':
            Tool('cc')(env)
            Tool('cxx')(env)
            Tool('link')(env)
            Tool('ar')(env)
            Tool('as')(env)
            compiler_mode = "unknown"

        elif compiler in ['hp', 'sgi', 'sun', 'aix']:
            Tool(compiler + 'cc')(env)
            Tool(compiler + 'c++')(env)
            Tool(compiler + 'link')(env)

            if compiler in ['sgi', 'sun']:
                Tool(compiler + 'ar')(env)

            compiler_mode = "unknown"

        elif compiler != 'default':
            Tool(compiler)(env)


    if compiler_mode is None:
        if env['CC'] == 'cl' or env['CC'] == 'icl': compiler_mode = 'msvc'
        elif env['CC'] == 'gcc' or env['CC'] == 'icc': compiler_mode = 'gnu'
        else: compiler_mode = 'unknown'

    env.__setitem__('compiler', compiler)
    env.__setitem__('compiler_mode', compiler_mode)

    if compiler == 'default':
        cc = env['CC']
        if cc == 'cl': compiler = 'msvc'
        elif cc == 'gcc': compiler = 'gnu'
        elif cc == 'icl' or cc == 'icc': compiler = 'intel'


    print "  Compiler: " + env['CC'] + ' (%s)' % compiler
    print "  Platform: " + env['PLATFORM']
    print "  Mode: " + compiler_mode

    # User flags
    if ccflags: env.Append(CCFLAGS = ccflags.split())
    if linkflags: env.Append(LINKFLAGS = linkflags.split())

    # Exceptions
    if compiler_mode == 'msvc':
        env.AppendUnique(CCFLAGS = ['/EHa']) # Asynchronous


    # Disable troublesome warnings
    warnings = []
    if compiler_mode == 'msvc':
        env.CBDefine('_CRT_SECURE_NO_WARNINGS')

        warnings += [4297, 4103]
        if compiler == 'intel': warnings += [1786]

    if compiler == 'intel': warnings += [279]

    if compiler == 'intel':
        warnings = map(str, warnings)
        if compiler_mode == 'msvc':
            env.AppendUnique(CCFLAGS = ['/Qdiag-disable:' + ','.join(warnings)])
        else:
            env.AppendUnique(CCFLAGS = ['-diag-disable', ','.join(warnings)])

    elif compiler_mode == 'msvc':
        for warning in warnings: env.AppendUnique(CCFLAGS = ['/wd%d' % warning])


    # Profiler flags
    if profile:
        if compiler_mode == 'gnu':
            env.AppendUnique(CCFLAGS = ['-pg'])
            env.AppendUnique(LINKFLAGS = ['-pg'])


    # Debug flags
    if compiler_mode == 'msvc':
        env['PDB'] = '${TARGET}.pdb'

    if debug:
        if compiler_mode == 'msvc':
            env.AppendUnique(CCFLAGS = ['/W1'])
            env['CCPDBFLAGS'] = '/Zi /Fd${TARGET}.pdb'
            env.AppendUnique(LINKFLAGS = ['/DEBUG', '/MAP:${TARGET}.map'])

        elif compiler_mode == 'gnu':
            if compiler == 'gnu':
                env.AppendUnique(CCFLAGS = ['-ggdb', '-Wall'])
                if conf.CheckRDynamic():
                    env.AppendUnique(LINKFLAGS = ['-rdynamic']) # for backtrace
            elif compiler == 'intel':
                env.AppendUnique(CCFLAGS = ['-g', '-diag-enable', 'warn'])

            if strict: env.AppendUnique(CCFLAGS = ['-Werror'])

        env.CBDefine('DEBUG')

        if not optimize and compiler == 'intel':
            if compiler_mode == 'gnu':
                env.AppendUnique(CCFLAGS = ['-mia32'])
            elif compiler_mode == 'msvc':
                env.AppendUnique(CCFLAGS = ['/arch:IA32'])

    else:
        if compiler_mode == 'gnu':
            # Don't add debug info and enable dead code removal
            env.AppendUnique(LINKFLAGS = ['-Wl,-S', '-Wl,-x'])

        env.CBDefine('NDEBUG')


    # Optimizations
    if optimize:
        if compiler_mode == 'gnu':
            env.AppendUnique(CCFLAGS = ['-O3', '-funroll-loops'])
            if compiler != 'intel' and compiler != 'clang':
                env.AppendUnique(CCFLAGS = ['-ffast-math', '-mfpmath=sse',
                                            '-fno-unsafe-math-optimizations'])
        elif compiler_mode == 'msvc':
            env.AppendUnique(CCFLAGS = ['/Ox'])
            if compiler == 'intel' and not globalopt:
                env.AppendUnique(LINKFLAGS = ['/Qnoipo'])

        # Whole program optimizations
        if globalopt:
            if compiler == 'intel':
                if compiler_mode == 'gnu':
                    env.AppendUnique(LINKFLAGS = ['-ipo'])
                    env.AppendUnique(CCFLAGS = ['-ipo'])
                elif compiler_mode == 'msvc':
                    env.AppendUnique(LINKFLAGS = ['/Qipo'])
                    env.AppendUnique(CCFLAGS = ['/Qipo'])

            elif compiler == 'msvc':
                env.AppendUnique(CCFLAGS = ['/GL'])
                env.AppendUnique(LINKFLAGS = ['/LTCG'])
                env.AppendUnique(ARFLAGS = ['/LTCG'])

        # Instruction set optimizations
        if sse2: opt_base, opt_auto = 'SSE2', 'SSE3,SSSE3,SSE4.1,SSE4.2'
        elif sse3: opt_base, opt_auto = 'SSE3', 'SSSE3,SSE4.1,SSE4.2'
        elif architecture()[0] == '64bit':
            opt_base, opt_auto = 'SSE2', 'SSE3,SSSE3,SSE4.1,SSE4.2'
        else: opt_base, opt_auto = 'SSE', 'SSE2,SSE3,SSSE3,SSE4.1,SSE4.2'

        if compiler_mode == 'gnu':
            env.AppendUnique(CCFLAGS = ['-m' + opt_base.lower()])

            if compiler == 'intel' and auto_dispatch:
                env.AppendUnique(CCFLAGS = ['-ax' + opt_auto])

        elif compiler_mode == 'msvc':
            env.AppendUnique(CCFLAGS = ['-arch:' + opt_base])

            if compiler == 'intel' and auto_dispatch:
                env.AppendUnique(CCFLAGS = ['/Qax' + opt_auto])

        # Intel threading optimizations
        if compiler == 'intel':
            if compiler_mode == 'msvc':
                env.AppendUnique(CCFLAGS = ['/Qopenmp'])
            else:
                env.AppendUnique(CCFLAGS = ['-openmp'])

            if compiler_mode == 'msvc': env.PrependUnique(LIBS = ['libiomp5mt'])
            else: env.PrependUnique(LIBS = ['iomp5'])
            

    # Pointer disambiguation
    if compiler == 'intel':
        if compiler_mode == 'gnu':
            env.AppendUnique(CCFLAGS = ['-restrict'])
        elif compiler_mode == 'msvc':
            env.AppendUnique(CCFLAGS = ['/Qrestrict'])


    # Dependency files
    if depends and compiler_mode == 'gnu':
        env.AppendUnique(CCFLAGS = ['-MMD -MF ${TARGET}.d'])


    # C mode
    if cstd:
        if compiler_mode == 'gnu':
            env.AppendUnique(CFLAGS = ['-std=' + cstd])
        elif compiler_mode == 'msvc' and compiler == 'intel':
            env.AppendUnique(CFLAGS = ['/Qstd=' + cstd])

    # C++ mode
    if cxxstd:
        if compiler_mode == 'gnu':
            env.AppendUnique(CXXFLAGS = ['-std=' + cxxstd])


    # Threads
    if threaded:
        if compiler_mode == 'gnu':
            conf.CBRequireLib('pthread')

            env.AppendUnique(LINKFLAGS = ['-pthread'])
            env.CBDefine('_REENTRANT')

        elif compiler_mode == 'msvc':
            if win32_thread == 'static':
                if debug: env.AppendUnique(CCFLAGS = ['/MTd'])
                else: env.AppendUnique(CCFLAGS = ['/MT'])
            else:
                if debug: env.AppendUnique(CCFLAGS = ['/MDd'])
                else: env.AppendUnique(CCFLAGS = ['/MD'])


    # Link flags
    if compiler_mode == 'msvc' and not optimize:
        env.AppendUnique(LINKFLAGS = ['/INCREMENTAL'])

    # static
    if static:
        if compiler_mode == 'gnu' and env['PLATFORM'] != 'darwin':
            env.AppendUnique(LINKFLAGS = ['-static'])


    # Num jobs default
    default_num_jobs = 1

    # distcc
    if distcc and compiler == 'gnu':
        default_num_jobs = 2
        env.Replace(CC = 'distcc ' + env['CC'])
        env.Replace(CXX = 'distcc ' + env['CXX'])

    # cccache
    if ccache and compiler == 'gnu':
        env.Replace(CC = 'ccache ' + env['CC'])
        env.Replace(CXX = 'ccache ' + env['CXX'])

    # Num jobs
    if num_jobs == -1:
        if os.environ.has_key('SCONS_JOBS'):
            num_jobs = int(os.environ.get('SCONS_JOBS', num_jobs))
        else: num_jobs = default_num_jobs

    SetOption('num_jobs', num_jobs)
    print "running with -j", GetOption('num_jobs')


    # For darwin
    if env['PLATFORM'] == 'darwin':
        env.CBDefine('__APPLE__')
        if osx_archs and compiler == 'gnu':
            # note: only apple compilers support multipe -arch options
            for arch in osx_archs.split():
                env.Append(CCFLAGS = ['-arch', arch])
                env.Append(LINKFLAGS = ['-arch', arch])
        if osx_min_ver:
            flag = '-mmacosx-version-min=' + osx_min_ver
            env.AppendUnique(LINKFLAGS = [flag])
            env.AppendUnique(CCFLAGS = [flag])
        if osx_sdk_root:
            env.Append(CCFLAGS = ['-isysroot', osx_sdk_root])
            # intel linker allegedly requires -isyslibroot
            # but that seems to confuse gcc
            if compiler == 'gnu':
                env.Append(LINKFLAGS = ['-isysroot', osx_sdk_root])
            elif compiler == 'intel':
                # untested
                #env.Append(LINKFLAGS = ['-Wl,-isyslibroot,' + osx_sdk_root])
                pass


def get_lib_path_env(env):
    eenv = copy.copy(os.environ)

    path = []

    if 'LIBPATH' in env: path += list(env['LIBPATH'])
    if 'LIBRARY_PATH' in eenv:
        path += eenv['LIBRARY_PATH'].split(':')

    eenv['LIBRARY_PATH'] = ':'.join(path)

    return eenv


def FindLibPath(env, lib):
    if env.get('compiler_mode', '') != 'gnu': return

    if lib.startswith(os.sep) or lib.endswith(env['LIBSUFFIX']):
        return lib # Already a path

    eenv = get_lib_path_env(env)
    cmd = env['CXX'].split()
    libpat = env['LIBPREFIX'] + lib + env['LIBSUFFIX']

    path = Popen(cmd + ['-print-file-name=' + libpat],
                 stdout = PIPE, env = eenv).communicate()[0].strip()

    if path != libpat: return path


def build_pattern(env, name):
    pats = env.get(name)
    if isinstance(pats, str): pats = pats.split()
    pats += env[name.upper()]

    return env.CBBuildSetRegex(pats)


def prefer_static_libs(env):
    if env.get('compiler_mode', '') != 'gnu': return

    mostly_static = env.get('mostly_static')
    prefer_static = build_pattern(env, 'prefer_static')
    prefer_dynamic = build_pattern(env, 'prefer_dynamic')
    require_static = build_pattern(env, 'require_static')

    libs = []
    changed = False

    for lib in env['LIBS']:
        name = str(lib)

        if require_static.match(name) or prefer_static.match(name) or \
                (mostly_static and not prefer_dynamic.match(name)):
            path = FindLibPath(env, name)
            if path is not None:
                changed = True
                libs.append(File(path))
                continue

        if require_static.match(name):
            raise Exception, 'Failed to find static library for "%s"' % name

        libs.append(lib)

    if changed:
        env.Replace(LIBS = libs)

        # Force two pass link to resolve circular dependencies
        if env['PLATFORM'] == 'posix':
            env['_LIBFLAGS'] = \
                '-Wl,--start-group ' + env['_LIBFLAGS'] + ' -Wl,--end-group'


def generate(env):
    env.CBAddConfigTest('compiler', configure)
    env.CBAddTest(CheckRDynamic)
    env.CBAddConfigFinishCB(prefer_static_libs)

    env.SetDefault(PREFER_DYNAMIC = 'pthread dl'.split())
    env.SetDefault(PREFER_STATIC = [])
    env.SetDefault(REQUIRE_STATIC = [])

    env.CBAddVariables(
        ('optimize', 'Enable or disable optimizations', -1),
        ('globalopt', 'Enable or disable global optimizations', 0),
        ('sse2', 'Enable SSE2 instructions', 0),
        ('sse3', 'Enable SSE3 instructions', 0),
        ('auto_dispatch', 'Enable auto-dispatch of optimized code paths', 1),
        BoolVariable('debug', 'Enable or disable debug options',
                     os.getenv('DEBUG_MODE', 0)),
        BoolVariable('strict', 'Enable or disable strict options', 1),
        BoolVariable('threaded', 'Enable or disable thread support', 1),
        BoolVariable('profile', 'Enable or disable profiler', 0),
        BoolVariable('depends', 'Enable or disable dependency files', 0),
        BoolVariable('distcc', 'Enable or disable distributed builds', 0),
        BoolVariable('ccache', 'Enable or disable cached builds', 0),
        EnumVariable('platform', 'Override default platform', '',
                   allowed_values = ('', 'win32', 'posix', 'darwin')),
        ('ccflags', 'Set extra C and C++ compiler flags', None),
        ('linkflags', 'Set extra linker flags', None),
        EnumVariable('cxxstd', 'Set C++ language standard', 'gnu++98',
                   allowed_values = ('gnu++98', 'c++98', 'c++0x', 'gnu++0x',
                                     'c++11', 'gnu++11')),
        EnumVariable('compiler', 'Select compiler', 'default',
                   allowed_values = ('default', 'gnu', 'intel', 'mingw', 'msvc',
                                     'linux-mingw', 'aix', 'posix', 'hp', 'sgi',
                                     'sun', 'clang')),
        BoolVariable('static', 'Link to static libraries', 0),
        BoolVariable('mostly_static', 'Prefer static libraries', 0),
        ('prefer_static', 'Libraries where the static version is prefered', ''),
        ('require_static', 'Libraries which must be linked statically', ''),
        ('prefer_dynamic', 'Libraries where the dynamic version is prefered, ' +
         'regardless of "mostly_static"', ''),
        ('num_jobs', 'Set the concurrency level.', -1),
        ('osx_min_ver', 'Set minimum support OSX version.', '10.6'),
        ('osx_sdk_root', 'Set OSX SDK root.', None),
        ('osx_archs', 'Set OSX gcc target architectures.', 'x86_64'),
        EnumVariable('win32_thread', 'Windows thread mode.', 'static',
                     allowed_values = ('static', 'dynamic')),
        BoolVariable('cross_mingw', 'Enable mingw cross compile mode', 0)
        )


def exists():
    return 1

