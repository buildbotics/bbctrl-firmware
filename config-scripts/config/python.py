from SCons.Script import *
import inspect
import traceback
import config



def add_vars(vars):
    vars.Add(BoolVariable('python', 'Set to 0 to disable python', 1)),
    vars.Add('python_version', 'Set python version', ''),


def try_config(conf, command):
    env = conf.env

    try:
        env.ParseConfig(command)
        env.Prepend(LIBS = ['util', 'm', 'dl', 'z'])

    except OSError:
        return False

    if conf.CheckHeader('Python.h') and conf.CheckFunc('Py_Initialize'):
        env.ParseConfig(command)
        env.AppendUnique(CPPDEFINES = ['HAVE_PYTHON']);
        env.Prepend(LIBS = ['util', 'm', 'dl', 'z'])

        return True

    return False
    

def configure(conf):
    env = conf.env

    if not env.get('python', 0): return False

    python_version = env.get('python_version', '')

    cmd = 'python%s-config' % python_version
    cmd = cmd + ' --ldflags; ' + cmd + ' --includes'

    if try_config(conf, cmd): return True

    dir = os.path.dirname(inspect.getfile(inspect.currentframe()))

    print "Trying local python-config.py"
    cmd = "python%s '%s'/python-config.py" % (python_version, dir)

    return try_config(conf, cmd)
