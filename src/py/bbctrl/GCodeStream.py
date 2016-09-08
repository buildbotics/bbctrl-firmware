import re
import logging


log = logging.getLogger('GCode')


class GCodeStream():
    comment1RE = re.compile(r';.*')
    comment2RE = re.compile(r'\(([^\)]*)\)')


    def __init__(self, path):
        self.path = path
        self.f = None

        self.open()


    def close(self):
        if self.f is not None:
            self.f.close()
            self.f = None


    def open(self):
        self.close()

        self.line = 0
        self.f = open('upload' + self.path, 'r')


    def reset(self): self.open()


    def comment(self, s):
        log.debug('Comment: %s', s)


    def next(self):
        line = self.f.readline()
        if line is None or line == '': return

        # Remove comments
        line = self.comment1RE.sub('', line)

        for comment in self.comment2RE.findall(line):
            self.comment(comment)

        line = self.comment2RE.sub(' ', line)

        # Remove space
        line = line.strip()

        # Append line number
        self.line += 1
        line += ' N%d' % self.line

        return line
