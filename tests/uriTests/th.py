import os

class Suite:
    def __init__(self, th):
	cmd = os.path.abspath(th.path + '/uri')

        th.Test('PlainURL', command = [cmd, 'http://www.example.com'])

        th.Test('AbsolutePath1', command = [cmd, '/'])
        th.Test('AbsolutePath2', command = [cmd, '/path/to/somefile.txt'])

        th.Test('Query1', command =
                [cmd, 'http://example.com/cgi-bin/test.cgi?name=value'])
        th.Test('Query2', command = [cmd, 'http://example.com/cgi-bin/' \
                    'test.cgi?name1=value1&name2=value2'])
        th.Test('Query3', command =
                [cmd, 'http://example.com/cgi-bin/test.cgi?name='])
        th.Test('Query4', command =
                [cmd, 'http://example.com/cgi-bin/test.cgi?name=&name2=value'])
        th.Test('Query5', command = [cmd,
                'http://example.com/cgi-bin/test.cgi?name1&name2=value&name3'])

        th.Test('User', command = [cmd, 'http://joe@example.com'])
        th.Test('UserPass', command = [cmd, 'http://joe:secret@example.com'])

        th.Test('OldClientBadURL', command = [cmd, 'http:/127.0.0.1:8080/'])
