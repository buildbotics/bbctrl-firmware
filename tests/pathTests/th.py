import os
import glob

class Suite:
    def __init__(self, th):
        prog = os.path.abspath(th.path + '/path')

        for test in glob.glob(th.path + '/*Test'):
            cmd = [prog]

            args = test + '/data/args'
            if os.path.exists(args):
                cmd += open(args).read().split()

            th.Test(os.path.basename(test), command = cmd)

