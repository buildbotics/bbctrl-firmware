#!/usr/bin/env python3

################################################################################
#                                                                              #
#                This file is part of the Buildbotics firmware.                #
#                                                                              #
#                  Copyright (c) 2015 - 2018, Buildbotics LLC                  #
#                             All rights reserved.                             #
#                                                                              #
#     This file ("the software") is free software: you can redistribute it     #
#     and/or modify it under the terms of the GNU General Public License,      #
#      version 2 as published by the Free Software Foundation. You should      #
#      have received a copy of the GNU General Public License, version 2       #
#     along with the software. If not, see <http://www.gnu.org/licenses/>.     #
#                                                                              #
#     The software is distributed in the hope that it will be useful, but      #
#          WITHOUT ANY WARRANTY; without even the implied warranty of          #
#      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU       #
#               Lesser General Public License for more details.                #
#                                                                              #
#       You should have received a copy of the GNU Lesser General Public       #
#                License along with the software.  If not, see                 #
#                       <http://www.gnu.org/licenses/>.                        #
#                                                                              #
#                For information regarding this software email:                #
#                  "Joseph Coffland" <joseph@buildbotics.com>                  #
#                                                                              #
################################################################################

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
