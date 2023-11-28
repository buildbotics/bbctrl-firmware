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

from .ProgramMDI import *

__all__ = ['ProgramHome']


# Axis homing procedure:
#
#   Mark axis unhomed
#   Set feed rate to search_vel
#   Seek closed by search_dist
#   Set feed rate to latch_vel
#   Seek open latch_backoff
#   Seek closed latch_backoff * -1.5
#   Rapid to zero_backoff
#   Mark axis homed and set absolute position

axis_homing_procedure = '''
  G28.2 %(axis)s0 F[#<_%(axis)s_search_velocity>]
  G38.6 %(axis)s[#<_%(axis)s_home_travel>]
  G38.8 %(axis)s[#<_%(axis)s_latch_backoff>] F[#<_%(axis)s_latch_velocity>]
  G38.6 %(axis)s[#<_%(axis)s_latch_backoff> * -8]
  G91 G0 G53 %(axis)s[#<_%(axis)s_zero_backoff>]
  G90 G28.3 %(axis)s[#<_%(axis)s_home_position>]
'''

stall_homing_procedure = '''
  G28.2 %(axis)s0 F[#<_%(axis)s_search_velocity>]
  G38.6 %(axis)s[#<_%(axis)s_home_travel>]
  G91 G0 G53 %(axis)s[#<_%(axis)s_zero_backoff>]
  G90 G28.3 %(axis)s[#<_%(axis)s_home_position>]
'''


class ProgramHome(ProgramMDI):
  status = 'homing'


  def __init__(self, ctrl, axis, position):
    state = ctrl.state
    log = ctrl.log.get('Home')
    mdi = ''

    if axis is None: axes = 'zxyabc' # TODO This should be configurable
    else: axes = '%c' % axis

    for axis in axes:
      enabled = state.is_axis_enabled(axis)
      mode = state.axis_homing_mode(axis)

      # If this is not a request to home a specific axis and the
      # axis is disabled or in manual homing mode, don't show any
      # warnings
      if 1 < len(axes) and (not enabled or mode == 'manual'):
        continue

      # Error when axes cannot be homed
      reason = state.axis_home_fail_reason(axis)
      if reason is not None:
        log.error('Cannot home %s axis: %s' % (axis.upper(), reason))
        continue

      if mode == 'manual':
        if position is None: raise Exception('Position not set')
        mdi += 'G28.3 %c%f\n' % (axis, position)
        continue

      # Home axis
      log.info('Homing %s axis' % axis)
      if mode.startswith('stall-'): procedure = stall_homing_procedure
      else: procedure = axis_homing_procedure
      mdi += procedure % {'axis': axis}

    super().__init__(ctrl, mdi, False)
