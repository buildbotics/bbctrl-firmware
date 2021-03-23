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
import sys
import signal
import tornado
import argparse
import datetime

from pkg_resources import Requirement, resource_filename

from bbctrl.RequestHandler import RequestHandler
from bbctrl.APIHandler import APIHandler
from bbctrl.FileSystemHandler import FileSystemHandler
from bbctrl.FileSystem import FileSystem
from bbctrl.ProgramQueue import ProgramQueue
from bbctrl.Config import Config
from bbctrl.LCD import LCD, LCDPage
from bbctrl.Mach import Mach
from bbctrl.Web import Web
from bbctrl.Jog import Jog
from bbctrl.Ctrl import Ctrl
from bbctrl.Pwr import Pwr
from bbctrl.I2C import I2C
from bbctrl.Planner import Planner
from bbctrl.Preplanner import Preplanner
from bbctrl.State import State
from bbctrl.Comm import Comm
from bbctrl.CommandQueue import CommandQueue
from bbctrl.MainLCDPage import MainLCDPage
from bbctrl.IPLCDPage import IPLCDPage
from bbctrl.Camera import Camera, VideoHandler
from bbctrl.AVR import AVR
from bbctrl.AVREmu import AVREmu
from bbctrl.IOLoop import IOLoop
from bbctrl.Events import Events
from bbctrl.MonitorTemp import MonitorTemp
import bbctrl.Cmd as Cmd
import bbctrl.v4l2 as v4l2
import bbctrl.Log as log
import bbctrl.ObjGraph as ObjGraph
import bbctrl.camotics as camotics


ctrl = None


def get_resource(path):
    return resource_filename(Requirement.parse('bbctrl'), 'bbctrl/' + path)


def on_exit(sig = 0, func = None):
    global ctrl

    print('Exit handler triggered: signal = %d', sig)

    if ctrl is not None:
        ctrl.close()
        ctrl = None

    sys.exit(0)


def time_str():
    return datetime.datetime.now().strftime('%Y%m%d-%H:%M:%S')



class Debugger:
    def __init__(self, ioloop, freq = 60 * 15, depth = 100):
        self.ioloop = ioloop
        self.freq = freq
        self.depth = depth
        self._callback()


    def _callback(self):
        with open('bbctrl-debug-%s.log' % time_str(), 'w') as log:
            def line(name):
                log.write('==== ' + name + ' ' + '=' * (74 - len(name)) + '\n')

            line('Common')
            ObjGraph.show_most_common_types(limit = self.depth, file = log)

            log.write('\n')
            line('Growth')
            ObjGraph.show_growth(limit = self.depth, file = log)

            log.write('\n')
            line('New IDs')
            ObjGraph.get_new_ids(limit = self.depth, file = log)

            log.flush()
            self.ioloop.call_later(self.freq, self._callback)



def parse_args():
    parser = argparse.ArgumentParser(
        description = 'Buildbotics Machine Controller')

    parser.add_argument('-p', '--port', default = 80,
                        type = int, help = 'HTTP port')
    parser.add_argument('-a', '--addr', metavar = 'IP', default = '0.0.0.0',
                        help = 'HTTP address to bind')
    parser.add_argument('-s', '--serial', default = '/dev/ttyAMA0',
                        help = 'Serial device')
    parser.add_argument('-b', '--baud', default = 230400, type = int,
                        help = 'Serial baud rate')
    parser.add_argument('--i2c-port', default = 1, type = int,
                        help = 'I2C port')
    parser.add_argument('--lcd-addr', default = [0x27, 0x3f], type = int,
                        help = 'LCD I2C address')
    parser.add_argument('--avr-addr', default = 0x2b, type = int,
                        help = 'AVR I2C address')
    parser.add_argument('--pwr-addr', default = 0x60, type = int,
                        help = 'Power AVR I2C address')
    parser.add_argument('-v', '--verbose', action = 'store_true',
                        help = 'Verbose output')
    parser.add_argument('-l', '--log', metavar = "FILE",
                        help = 'Set a log file')
    parser.add_argument('--disable-camera', action = 'store_true',
                        help = 'Disable the camera')
    parser.add_argument('--width', default = 640, type = int,
                        help = 'Camera width')
    parser.add_argument('--height', default = 480, type = int,
                        help = 'Camera height')
    parser.add_argument('--fps', default = 15, type = int,
                        help = 'Camera frames per second')
    parser.add_argument('--camera-clients', default = 4,
                        help = 'Maximum simultaneous camera clients')
    parser.add_argument('--demo', action = 'store_true',
                        help = 'Enter demo mode')
    parser.add_argument('--debug', default = 0, type = int,
                        help = 'Enable debug mode and set frequency in seconds')
    parser.add_argument('--fast-emu', action = 'store_true',
                        help = 'Enter demo mode')
    parser.add_argument('--client-timeout', default = 5 * 60, type = int,
                        help = 'Demo client timeout in seconds')

    return parser.parse_args()


def run():
    global ctrl

    args = parse_args()

    # Set signal handler
    signal.signal(signal.SIGTERM, on_exit)

    # Create ioloop
    ioloop = tornado.ioloop.IOLoop.current()

    # Set ObjGraph signal handler
    if args.debug: Debugger(ioloop, args.debug)

    # Set TPL path
    os.environ['TPL_PATH'] = \
        '/var/lib/bbctrl/upload/lib/:' + get_resource('tpl_lib/')

    # Start server
    web = Web(args, ioloop)

    try:
        ioloop.start()

    except KeyboardInterrupt: on_exit()


if __name__ == '__main__': run()
