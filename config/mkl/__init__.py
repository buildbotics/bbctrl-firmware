import sys
from SCons.Script import *
from platform import machine, architecture


def CheckMKL(ctx):
    env = ctx.env
    conf = ctx.sconf

    ctx.Message('Checking for Intel MKL... ')

    # Save
    save_vars = 'LIBPATH CPPPATH LIBS CPPDEFINES SHLINKCOM LINKCOM'
    save = {}
    for name in save_vars.split(): save[name] = env.get(name, None)

    config.check_home(conf, 'mkl', suffix = 'ROOT')

    # Test program
    source = """
      #include "mkl.h"
      int main()
      {
        char ta = 'N', tb = 'N';
        #ifdef MKL_ILP64
        long long
        #endif
        int M = 1, N = 1, K = 1, lda = 1, ldb = 1, ldc = 1;
        double alpha = 1.0, beta = 1.0, *A = 0, *B = 0, *C = 0;
        dgemm(&ta, &tb, &M, &N, &K, &alpha, A, &lda, B, &ldb, &beta, C, &ldc);
        return 0;
      }
    """
    source = source.strip()

    env.PrependUnique(LIBS = ['m'])

    # Compile & Link options
    compiler = env.get('compiler')
    if compiler == 'intel':
        if env['PLATFORM'] == 'win32':
            env.AppendUnique(CCFLAGS = ['/Qmkl'])
            env.AppendUnique(LINKFLAGS = ['/Qmkl'])
        else:
            env.AppendUnique(CCFLAGS = ['-mkl'])
            env.AppendUnique(LINKFLAGS = ['-mkl'])

    else:
        if architecture()[0] == '64bit': suffix = '_lp64'
        elif env['PLATFORM'] == 'win32': suffix = '_c'
        else: suffix = ''
        mkl_libs = ['mkl_intel' + suffix, 'mkl_intel_thread', 'mkl_core']

        if int(env.get('static')):
            env['MKL_LIBS'] = '-l' + ' -l'.join(mkl_libs)
            mkl_lib_flags = ' -Wl,--start-group $MKL_LIBS -Wl,--end-group'

        else: env.Prepend(LIBS = mkl_libs)

        libs = []
        if env['PLATFORM'] == 'win32': libs += ['libiomp5mt']
        else: libs += ['iomp5']

        if compiler == 'gnu': libs += ['pthread', 'm']

        if env.get('static', 0):
            mkl_lib_flags += ' -l' + ' -l'.join(libs)

            env.Append(SHLINKCOM = mkl_lib_flags)
            env.Append(LINKCOM = mkl_lib_flags)

        else: env.Prepend(LIBS = libs)

    # Try it
    if ctx.TryCompile(source, '.cpp') and ctx.TryLink(source, '.cpp'):
        env.CBDefine('HAVE_MKL')
        ctx.Result(True)
        return True

    # Restore
    env.Replace(**save)

    ctx.Result(False)
    return False


def RequireMKL(ctx):
    if not CheckMKL(ctx): raise Exception, 'Need MKL'
    return True


def configure(conf):
    conf.CheckMKL()


def generate(env):
    env.CBAddTest(CheckMKL)
    env.CBAddTest(RequireMKL)
    env.CBAddConfigTest('mkl', configure)


def exists(env):
    return 1
