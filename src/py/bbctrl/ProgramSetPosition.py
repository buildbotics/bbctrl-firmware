################################################################################
#                                                                              #
#                 This file is part of the Buildbotics firmware.               #
#                                                                              #
#        Copyright (c) 2015 - 2022, Buildbotics LLC, All rights reserved.      #
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

from .ProgramMDI import *

__all__ = ['ProgramSetPosition']


class ProgramSetPosition(ProgramMDI):
  def __init__(self, ctrl, axis, position):
    axis  = axis.lower()
    state = ctrl.state

    if state.is_axis_homed(axis):
      # If homed, change the offset rather than the absolute position
      cmd = 'G92%s%f' % (axis, position)

    elif state.is_axis_enabled(axis):
      # Set absolute position via planner
      target = position + state.get('offset_' + axis)
      cmd = 'G28.3 %s%f\nG28.2 %s0' % (axis, target, axis)

    super().__init__(ctrl, cmd)
