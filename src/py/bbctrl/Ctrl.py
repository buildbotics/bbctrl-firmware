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
import time
import bbctrl


class Ctrl(object):
    def __init__(self, args, ioloop, id):
        self.args = args
        self.ioloop = bbctrl.IOLoop(ioloop)
        self.id = id
        self.timeout = None # Used in demo mode

        if id:
            if not os.path.exists(id): os.mkdir(id)
            self.root = './' + id
        else: self.root = '.'

        # Start log
        if args.demo: log_path = self.get_path(filename = 'bbctrl.log')
        else: log_path = args.log
        self.log = bbctrl.log.Log(args, self.ioloop, log_path)

        self.events = bbctrl.Events(self)
        self.state  = bbctrl.State(self)
        self.config = bbctrl.Config(self)

        self.log.get('Ctrl').info('Starting %s' % self.id)

        try:
            if args.demo: self.avr = bbctrl.AVREmu(self)
            else: self.avr = bbctrl.AVR(self)

            self.i2c = bbctrl.I2C(args.i2c_port, args.demo)
            self.lcd = bbctrl.LCD(self)
            self.mach = bbctrl.Mach(self, self.avr)
            self.preplanner = bbctrl.Preplanner(self)
            self.fs = bbctrl.FileSystem(self)
            if not args.demo: self.jog = bbctrl.Jog(self)
            self.pwr = bbctrl.Pwr(self)

            self.mach.connect()

            self.lcd.add_new_page(bbctrl.MainLCDPage(self))
            self.lcd.add_new_page(bbctrl.IPLCDPage(self.lcd))

        except Exception: self.log.get('Ctrl').exception()


    def __del__(self): print('Ctrl deleted')


    def clear_timeout(self):
        if self.timeout is not None: self.ioloop.remove_timeout(self.timeout)
        self.timeout = None


    def set_timeout(self, cb, *args, **kwargs):
        self.clear_timeout()
        t = self.args.client_timeout
        self.timeout = self.ioloop.call_later(t, cb, *args, **kwargs)


    def get_path(self, dir = None, filename = None):
        path = self.root if dir is None else (self.root + '/' + dir)
        return path if filename is None else (path + '/' + filename)


    def get_plan(self, filename = None):
        return self.get_path('plans', filename)


    def configure(self):
        # Called from Comm.py after AVR vars are loaded
        # Indirectly configures state via calls to config() and the AVR
        self.config.reload()
        self.state.init()


    def ready(self):
        # This is used to synchronize the start of the preplanner
        self.preplanner.start()


    def close(self):
        self.log.get('Ctrl').info('Closing %s' % self.id)
        self.ioloop.close()
        self.avr.close()
        self.mach.planner.close()
