#!/usr/bin/env python3

################################################################################
#                                                                              #
#                 This file is part of the Buildbotics firmware.               #
#                                                                              #
#        Copyright (c) 2015 - 2023, Buildbotics LLC, All rights reserved.      #
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

import pyudev
import logging


class UDevEvent:
  def __init__(self, ioloop, log = None):
    self.ioloop   = ioloop
    self.log      = log if log else logging.getLogger('UDev')
    self.udevCtx  = pyudev.Context()
    self.udevMon  = pyudev.Monitor.from_netlink(self.udevCtx)
    self.handlers = {}

    self.udevMon.start()
    ioloop.add_handler(self.udevMon.fileno(), self._handler, ioloop.READ)


  def add_handler(self, handler, subsystem = '*'):
    if not subsystem in self.handlers: self.handlers[subsystem] = []
    self.handlers[subsystem].append(handler)


  def dev_by_name(self, subsystem, name):
    return pyudev.Device.from_name(self.udevCtx, subsystem, name)


  def _handler(self, fd, events):
    action, device = self.udevMon.receive_device()
    if device is None: return

    match = False
    for subsystem in ['*', device.subsystem]:
      if subsystem in self.handlers:
        for handler in self.handlers[subsystem]:
          match = True
          handler(action, device)

    if match: self.log.info('%s: %s: %s' % (
        action, device.subsystem, device.device_node))



if __name__ == '__main__':
  import tornado
  import logging

  ioloop = tornado.ioloop.IOLoop.current()
  udevev = UDevEvent(ioloop, logging.getLogger('UDev'))

  def print_dev(action, dev):
    print(action, dev.subsystem, dev.device_node, dev.device_path)

  udevev.add_handler(print_dev, 'block')

  try:
    ioloop.start()
  except KeyboardInterrupt: pass
