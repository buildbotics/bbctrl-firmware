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

import time
import os


def read_temp():
    path = '/sys/class/thermal/thermal_zone0/temp'
    if not os.path.exists(path): return 0

    try:
        with open(path, 'r') as f:
            return round(int(f.read()) / 1000)
    except:
        return 0


def set_max_freq(freq):
    path = '/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq'
    if not os.path.exists(path): return
    try:
        with open(path, 'w') as f: f.write('%d\n' % freq)
    except: pass


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

        except SystemExit: pass
        except: self.log.exception()

        self.ioloop.call_later(5, self.callback)
