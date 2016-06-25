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
from bbctrl.Config import Config
from bbctrl.LCD import LCD
from bbctrl.AVR import AVR
from bbctrl.Web import Web
from bbctrl.Jog import Jog
from bbctrl.Ctrl import Ctrl


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
    parser.add_argument('--lcd-port', default = 1, type = int,
                        help = 'LCD I2C port')
    parser.add_argument('--lcd-addr', default = 0x27, type = int,
                        help = 'LCD I2C address')
    parser.add_argument('-v', '--verbose', action = 'store_true',
                        help = 'Verbose output')
    parser.add_argument('-l', '--log', metavar = "FILE",
                        help = 'Set a log file')

    return parser.parse_args()


def run():
    args = parse_args()

    # Init logging
    log = logging.getLogger()
    log.setLevel(logging.DEBUG if args.verbose else logging.INFO)
    if args.log: log.addHandler(logging.FileHandler(args.log, mode = 'w'))

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
