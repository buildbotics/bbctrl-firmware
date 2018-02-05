#!/usr/bin/env python3

import os
import sys
import signal
import tornado
import argparse
import logging

from pkg_resources import Requirement, resource_filename

from bbctrl.APIHandler import APIHandler
from bbctrl.FileHandler import FileHandler
from bbctrl.GCodeStream import GCodeStream
from bbctrl.Config import Config
from bbctrl.LCD import LCD, LCDPage
from bbctrl.AVR import AVR
from bbctrl.Web import Web
from bbctrl.Jog import Jog
from bbctrl.Ctrl import Ctrl
from bbctrl.Pwr import Pwr
from bbctrl.I2C import I2C
from bbctrl.Planner import Planner
from bbctrl.State import State
import bbctrl.Cmd as Cmd


def get_resource(path):
    return resource_filename(Requirement.parse('bbctrl'), 'bbctrl/' + path)


def on_exit(sig = 0, func = None):
    logging.info('Exit handler triggered: signal = %d', sig)
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

    return parser.parse_args()


def run():
    args = parse_args()

    # Init logging
    root = logging.getLogger()
    root.setLevel(logging.DEBUG if args.verbose else logging.INFO)
    f = logging.Formatter('{levelname[0]}:{name}:{message}', style = '{')
    h = logging.StreamHandler()
    h.setFormatter(f)
    root.addHandler(h)

    if args.log:
        h = logging.FileHandler(args.log)
        h.setFormatter(f)
        root.addHandler(h)

    # Set signal handler
    signal.signal(signal.SIGTERM, on_exit)

    # Create ioloop
    ioloop = tornado.ioloop.IOLoop.current()

    # Start controller
    ctrl = Ctrl(args, ioloop)

    try:
        ioloop.start()

    except KeyboardInterrupt:
        on_exit()

    except: log.exception('')


if __name__ == "__main__": run()
