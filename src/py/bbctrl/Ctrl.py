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

import logging
import bbctrl


log = logging.getLogger('Ctrl')



class Ctrl(object):
    def __init__(self, args, ioloop):
        self.args = args
        self.ioloop = ioloop

        self.msgs = bbctrl.Messages(self)
        self.state = bbctrl.State(self)
        self.config = bbctrl.Config(self)
        self.web = bbctrl.Web(self)

        try:
            self.i2c = bbctrl.I2C(args.i2c_port)
            self.lcd = bbctrl.LCD(self)
            self.mach = bbctrl.Mach(self)
            self.preplanner = bbctrl.Preplanner(self)
            self.planTimer = bbctrl.PlanTimer(self)
            self.jog = bbctrl.Jog(self)
            self.pwr = bbctrl.Pwr(self)

            self.mach.connect()

            self.lcd.add_new_page(bbctrl.MainLCDPage(self))
            self.lcd.add_new_page(bbctrl.IPLCDPage(self.lcd))

            self.camera = bbctrl.Camera(self)

        except Exception as e: log.exception(e)


    def configure(self):
        # Indirectly configures state via calls to config() and the AVR
        self.config.reload()


    def ready(self):
        # This is used to synchronize the start of the preplanner
        self.preplanner.start()


    def close(self): self.camera.close()
