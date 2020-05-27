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

import bbctrl


class MainLCDPage(bbctrl.LCDPage):
    def __init__(self, ctrl):
        bbctrl.LCDPage.__init__(self, ctrl.lcd)

        self.ctrl = ctrl
        self.install = True

        ctrl.state.add_listener(self.update)


    def update(self, update):
        state = self.ctrl.state

        # Must be after machine vars have loaded
        if self.install and hasattr(self, 'id'):
            self.install = False
            self.ctrl.lcd.set_current_page(self.id)

        self.text('%-9s' % state.get('xx', ''), 0, 0)

        metric = not state.get('imperial', False)
        scale = 1 if metric else 25.4

        # Show enabled axes
        row = 0
        for axis in 'xyzabc':
            if state.is_axis_faulted(axis):
                self.text('    FAULT %s' % axis.upper(), 9, row)
                row += 1

            elif state.is_axis_enabled(axis):
                position = state.get(axis + 'p', 0)
                position += state.get('offset_' + axis, 0)
                position /= scale
                self.text('% 10.3f%s' % (position, axis.upper()), 9, row)
                row += 1

        while row < 4:
            self.text(' ' * 11, 9, row)
            row += 1

        # Show tool, units, feed and speed
        self.text('%2uT' % state.get('tool',  0),           6, 1)
        self.text('%-6s' % 'MM' if metric else 'INCH',      0, 1)
        self.text('%8uF' % (state.get('feed',  0) / scale), 0, 2)
        self.text('%8dS' % state.get('speed', 0),           0, 3)
