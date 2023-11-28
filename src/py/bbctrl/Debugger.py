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

from . import ObjGraph, util

__all__ = ['Debugger']


class Debugger:
  def __init__(self, ioloop, freq = 60 * 15, depth = 100):
    self.ioloop = ioloop
    self.freq   = freq
    self.depth  = depth
    self._callback()


  def _callback(self):
    with open('bbctrl-debug-%s.log' % util.timestamp(), 'w') as log:
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
