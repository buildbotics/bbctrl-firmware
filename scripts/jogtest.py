#!/usr/bin/env python3

import sys
import tornado.ioloop

from inevent import InEvent, JogHandler
from inevent.Constants import *


class Handler(JogHandler):
  def changed(self):
    scale = 1.0
    if self.speed == 1: scale = 1.0 / 128.0
    if self.speed == 2: scale = 1.0 / 32.0
    if self.speed == 3: scale = 1.0 / 4.0

    print(', '.join(list(map(lambda x: '%.3f' % (x * scale), self.axes))))
    sys.stdout.flush()


if __name__ == "__main__":
  # Load config
  config = {
      "Logitech Logitech RumblePad 2 USB": {
          "deadband": 0.1,
          "axes":     [ABS_X, ABS_Y, ABS_RZ, ABS_Z],
          "dir":      [1, -1, -1, 1],
          "arrows":   [ABS_HAT0X, ABS_HAT0Y],
          "speed":    [0x120, 0x121, 0x122, 0x123],
          "lock":     [0x124, 0x125],
          },

      "default": {
          "deadband": 0.1,
          "axes":     [ABS_X, ABS_Y, ABS_RY, ABS_RX],
          "dir":      [1, -1, -1, 1],
          "arrows":   [ABS_HAT0X, ABS_HAT0Y],
          "speed":    [0x133, 0x130, 0x131, 0x134],
          "lock":     [0x136, 0x137],
          }
      }

  # Create ioloop
  ioloop = tornado.ioloop.IOLoop.current()

  # Listen for input events
  handler = Handler(config)
  events = InEvent(ioloop, handler, types = "js kbd".split())

  ioloop.start()
