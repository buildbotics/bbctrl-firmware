#!/usr/bin/env python3

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

import os
import sys
import signal
import tornado
import argparse
import logging
import datetime

from pkg_resources import Requirement, resource_filename

from bbctrl.APIHandler import APIHandler
from bbctrl.FileHandler import FileHandler
from bbctrl.GCodeStream import GCodeStream
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
from bbctrl.Messages import Messages
from bbctrl.Comm import Comm
from bbctrl.CommandQueue import CommandQueue
from bbctrl.MainLCDPage import MainLCDPage
from bbctrl.IPLCDPage import IPLCDPage
from bbctrl.Camera import Camera, VideoHandler
from bbctrl.PlanTimer import PlanTimer
import bbctrl.Cmd as Cmd
import bbctrl.v4l2 as v4l2


ctrl = None


def get_resource(path):
    return resource_filename(Requirement.parse('bbctrl'), 'bbctrl/' + path)


def on_exit(sig = 0, func = None):
    global ctrl

    logging.info('Exit handler triggered: signal = %d', sig)

    if ctrl is not None:
        ctrl.close()
        ctrl = None

    sys.exit(1)


def parse_args():
    parser = argparse.ArgumentParser(
        description = 'Buildbotics Machine Controller')

    parser.add_argument('-p', '--port', default = 80,
                        type = int, help = 'HTTP port')
    parser.add_argument('-a', '--addr', metavar = 'IP', default = '0.0.0.0',
                        help = 'HTTP address to bind')
    parser.add_argument('-s', '--serial', default = '/dev/ttyAMA0',
                        help = 'Serial device')
    parser.add_argument('-b', '--baud', default = 115200, type = int,
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
    parser.add_argument('--width', default = 640, type = int,
                        help = 'Camera width')
    parser.add_argument('--height', default = 480, type = int,
                        help = 'Camera height')
    parser.add_argument('--fps', default = 15, type = int,
                        help = 'Camera frames per second')
    parser.add_argument('--fourcc', default = 'MJPG',
                        help = 'Camera frame format')

    return parser.parse_args()


def run():
    global ctrl

    args = parse_args()

    # Init logging
    root = logging.getLogger()
    level = logging.DEBUG if args.verbose else logging.INFO
    root.setLevel(logging.NOTSET)
    f = logging.Formatter('{levelname[0]}:{name}:{message}', style = '{')
    h = logging.StreamHandler()
    h.setLevel(level)
    h.setFormatter(f)
    root.addHandler(h)

    if args.log:
        h = logging.handlers.RotatingFileHandler(args.log, maxBytes = 1000000,
                                                 backupCount = 5)
        h.setLevel(level)
        h.setFormatter(f)
        root.addHandler(h)

    # Log header
    now = datetime.datetime.now().strftime('%Y/%m/%d %H:%M:%S')
    root.info('Log started ' + now)

    # Set signal handler
    signal.signal(signal.SIGTERM, on_exit)

    # Create ioloop
    ioloop = tornado.ioloop.IOLoop.current()

    # Start controller
    ctrl = Ctrl(args, ioloop)

    try:
        ioloop.start()

    except KeyboardInterrupt: on_exit()
    except SystemExit: raise
    except: logging.getLogger().exception('')


if __name__ == "__main__": run()
