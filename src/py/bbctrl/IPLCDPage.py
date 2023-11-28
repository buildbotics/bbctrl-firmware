################################################################################
#                                                                              #
#                 This file is part of the Buildbotics firmware.               #
#                                                                              #
#        Copyright (c) 2015 - 2023, Buildbotics LLC, All rights reserved.      #
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

import subprocess

from .LCDPage import *

__all__ = ['IPLCDPage']


class IPLCDPage(LCDPage):
    # From LCDPage
    def activate(self):
        p = subprocess.Popen(['hostname', '-I'], stdout = subprocess.PIPE)
        ips = p.communicate()[0].decode('utf-8').split()

        p = subprocess.Popen(['hostname'], stdout = subprocess.PIPE)
        hostname = p.communicate()[0].decode('utf-8').strip()

        self.clear()

        self.text('Host: %s' % hostname[0:14], 0, 0)

        for i in range(min(3, len(ips))):
            if len(ips[i]) <= 16:
                self.text('IP: %s' % ips[i], 0, i + 1)
