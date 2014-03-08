import os

class Suite:
    def __init__(self, th):
	cmd = os.path.abspath(th.path + '/bfencdec')
        th.Test('Blowfish', command = [cmd, 'test'])
