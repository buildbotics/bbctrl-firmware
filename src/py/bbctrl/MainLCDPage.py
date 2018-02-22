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
        # Must be after machine vars have loaded
        if self.install and hasattr(self, 'id'):
            self.install = False
            self.ctrl.lcd.set_current_page(self.id)

        if 'xx' in update:
            self.text('%-9s' % self.ctrl.state.get('xx'), 0, 0)

       # Show enabled axes
        row = 0
        for axis in 'xyzabc':
            motor = self.ctrl.state.find_motor(axis)
            if motor is not None:
                if (axis + 'p') in update:
                    self.text('% 10.3f%s' % (update[axis + 'p'], axis.upper()),
                              9, row)

                row += 1

        # Show tool, units, feed and speed
        if 'tool'  in update: self.text('%2uT' % update['tool'],  6, 1)
        if 'units' in update: self.text('%-6s' % update['units'], 0, 1)
        if 'feed'  in update: self.text('%8uF' % update['feed'],  0, 2)
        if 'speed' in update: self.text('%8dS' % update['speed'], 0, 3)
