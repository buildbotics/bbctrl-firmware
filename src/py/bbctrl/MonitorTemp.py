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

import time


def read_temp():
    with open('/sys/class/thermal/thermal_zone0/temp', 'r') as f:
        return round(int(f.read()) / 1000)


def set_max_freq(freq):
    filename = '/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq'
    with open(filename, 'w') as f: f.write('%d\n' % freq)


class MonitorTemp(object):
    def __init__(self, app):
        self.app = app

        self.ctrl = app.get_ctrl()
        self.log = self.ctrl.log.get('Mon')
        self.ioloop = self.ctrl.ioloop

        self.last_temp_warn = 0
        self.temp_thresh = 80
        self.min_temp = 60
        self.max_temp = 80
        self.min_freq = 600000
        self.max_freq = 1200000
        self.low_camera_temp = 75
        self.high_camera_temp = 80

        self.callback()


    # Scale max CPU based on temperature
    def scale_cpu(self, temp):
        if temp < self.min_temp: cpu_freq = self.max_freq
        elif self.max_temp < temp: cpu_freq = self.min_freq
        else:
            r = 1 - float(temp - self.min_temp) / \
                (self.max_temp - self.min_temp)
            cpu_freq = self.min_freq + (self.max_freq - self.min_freq) * r

        set_max_freq(cpu_freq)


    def update_camera(self, temp):
        if self.app.camera is None: return

        # Disable camera if temp too high
        if temp < self.low_camera_temp: self.app.camera.set_overtemp(False)
        elif self.high_camera_temp < temp:
            self.app.camera.set_overtemp(True)


    def log_warnings(self, temp):
        # Reset temperature warning threshold after timeout
        if time.time() < self.last_temp_warn + 60: self.temp_thresh = 80

        if self.temp_thresh < temp:
            self.last_temp_warn = time.time()
            self.temp_thresh = temp

            self.log.info('Hot RaspberryPi at %d°C' % temp)


    def callback(self):
        try:
            temp = read_temp()

            self.ctrl.state.set('rpi_temp', temp)
            self.scale_cpu(temp)
            self.update_camera(temp)
            self.log_warnings(temp)

        except: self.log.exception()

        self.ioloop.call_later(5, self.callback)
