import os

class Suite:
    def __init__(self, th):
	cmd = os.path.abspath(th.path + '/base64')

        th.Test('Encode1', command = [cmd, '-e', 'leasure.'])
        th.Test('Encode2', command = [cmd, '-e', 'easure.'])
        th.Test('Encode3', command = [cmd, '-e', 'asure.'])
        th.Test('Encode4', command = [cmd, '-e', 'sure.'])

        th.Test('Decode1', command = [cmd, '-d', 'bGVhc3VyZS4='])
        th.Test('Decode2', command = [cmd, '-d', 'ZWFzdXJlLg=='])
        th.Test('Decode3', command = [cmd, '-d', 'YXN1cmUu'])
        th.Test('Decode4', command = [cmd, '-d', 'c3VyZS4='])
