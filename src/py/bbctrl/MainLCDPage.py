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

        # Show enabled axes
        row = 0
        for axis in 'xyzabc':
            if state.is_axis_enabled(axis):
                position = state.get(axis + 'p', 0)
                self.text('% 10.3f%s' % (position, axis.upper()), 9, row)
                row += 1

        while row < 4:
            self.text(' ' * 11, 9, row)
            row += 1

        # Show tool, units, feed and speed
        units = 'INCH' if state.get('imperial', False) else 'MM'
        self.text('%2uT' % state.get('tool', 0), 6, 1)
        self.text('%-6s' % units,                0, 1)
        self.text('%8uF' % state.get('feed', 0), 0, 2)
        self.text('%8dS' % state.get('speed',0), 0, 3)
