from SCons.Script import *
import inspect
import traceback


def check_config(conf):
    env = conf.env
    conf.CBRequireHeader('Python.h')
    conf.CBRequireFunc('Py_Initialize')
    env.CBDefine('HAVE_PYTHON')
    env.Prepend(LIBS = ['util', 'm', 'dl', 'z'])
    return True


def try_config(conf, command):
    env = conf.env

    try:
        env.ParseConfig(command)
        env.Prepend(LIBS = ['util', 'm', 'dl', 'z'])

    except OSError:
        return False

    try:
        return check_config(conf)
    except:
        return False


def configure(conf):
    env = conf.env

    if not env.get('python', 0): return False

    home = conf.CBCheckHome('python', inc_suffix = '/Include /include')
    if home:
        env.AppendUnique(CPPPATH = [home])
        return check_config(conf)

    python_version = env.get('python_version', '')

    cmd = 'python%s-config' % python_version
    cmd = cmd + ' --ldflags; ' + cmd + ' --includes'

    if try_config(conf, cmd): return True

    dir = os.path.dirname(inspect.getfile(inspect.currentframe()))

    print "Trying local python-config.py"
    cmd = "python%s '%s'/python-config.py" % (python_version, dir)

    return try_config(conf, cmd)


def generate(env):
    env.CBAddConfigTest('python', configure)

    env.CBAddVariables(
        BoolVariable('python', 'Set to 0 to disable python', 1),
        ('python_version', 'Set python version', ''))



def exists():
    return 1

