#!/usr/bin/env python3

################################################################################
#                                                                              #
#                 This file is part of the Buildbotics firmware.               #
#                                                                              #
#        Copyright (c) 2015 - 2020, Buildbotics LLC, All rights reserved.      #
#                                                                              #
#         This Source describes Open Hardware and is licensed under the        #
#                                 CERN-OHL-S v2.                               #
#                                                                              #
#         You may redistribute and modify this Source and make products        #
#    using it under the terms of the CERN-OHL-S v2 (https:/cern.ch/cern-ohl).  #
#           This Source is distributed WITHOUT ANY EXPRESS OR IMPLIED          #
#    WARRANTY, INCLUDING OF MERCHANTABILITY, SATISFACTORY QUALITY AND FITNESS  #
#     FOR A PARTICULAR PURPOSE. Please see the CERN-OHL-S v2 for applicable    #
#                                  conditions.                                 #
#                                                                              #
#                Source location: https://github.com/buildbotics               #
#                                                                              #
#      As per CERN-OHL-S v2 section 4, should You produce hardware based on    #
#    these sources, You must maintain the Source Location clearly visible on   #
#    the external case of the CNC Controller or other product you make using   #
#                                  this Source.                                #
#                                                                              #
#                For more information, email info@buildbotics.com              #
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
