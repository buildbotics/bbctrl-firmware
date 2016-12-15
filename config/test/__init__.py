from SCons.Script import *
import subprocess
import sys
import shlex


def run_tests(env):
    subprocess.call(shlex.split(env.get('TEST_COMMAND')))
    sys.exit(0)


def generate(env):
    env.CBAddVariables(('TEST_COMMAND', 'Command to run for `test` target',
                        'tests/testHarness -C tests --no-color --diff-failed '
                        '--view-failed --view-unfiltered --save-failed'))

    if 'test' in COMMAND_LINE_TARGETS: env.CBAddConfigFinishCB(run_tests)


def exists(): return 1
