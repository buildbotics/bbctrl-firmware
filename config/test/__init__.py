from SCons.Script import *


def run_tests(env):
    import shlex
    import subprocess
    import sys

    subprocess.call(shlex.split(env.get('TEST_COMMAND')))
    sys.exit(0)


def generate(env):
    import os

    cmd = 'tests/testHarness -C tests --diff-failed --view-failed ' \
        '--view-unfiltered --save-failed --build'

    if 'DOCKBOT_MASTER_PORT' in os.environ: cmd += ' --no-color'

    env.CBAddVariables(('TEST_COMMAND', '`test` target command line', cmd))

    if 'test' in COMMAND_LINE_TARGETS: env.CBAddConfigFinishCB(run_tests)


def exists(): return 1
