#!/usr/bin/env python3

import os
import re
import shlex
import subprocess


lineRE = r'%(addr)s '
command = 'avr-objdump -j .bss -t buildbotics.elf'

proc = subprocess.Popen(shlex.split(command), stdout = subprocess.PIPE)

out, err = proc.communicate()

if proc.returncode:
    print(out)
    raise Exception('command failed')

def get_sizes(data):
    for line in data.decode().split('\n'):
        if not re.match(r'^[0-9a-f]{8} .*', line): continue

        size, name  = int(line[22:30], 16), line[31:]
        if not size: continue

        yield (size, name)

sizes = sorted(get_sizes(out))
total = sum(x[0] for x in sizes)

for size, name in sizes:
    print('% 6d %5.2f%% %s' % (size, size / total * 100, name))

print('-' * 40)
print('% 6d Total' % total)
