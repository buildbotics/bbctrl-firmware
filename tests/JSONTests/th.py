import os

class Suite:
    def __init__(self, th):
	cmd = os.path.abspath(th.path + '/JSON')

        # TODO Test erorr cases
        for test in ['Boolean', 'Number', 'None', 'String', 'List', 'Dict',
                     'Single', 'General']:
            th.Test(test, command = [cmd])
