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

from .Program import *

__all__ = ['ProgramMDI']


class ProgramMDI(Program):
  status = 'mdi'


  def __init__(self, ctrl, cmd, with_limits = True):
    super().__init__(ctrl)

    self.cmd = cmd
    self.with_limits = with_limits
    self.log = self.ctrl.log.get('MDI')


  def _start_var_cmd(self, cmd):
    state = self.ctrl.state
    equal = cmd.find('=')

    if equal == -1:
      # Log state variable
      self.log.info('%s=%s' % (cmd, state.get(cmd[1:])))

    else:
      # Set state variable
      name, value = cmd[1:equal], cmd[equal + 1:]

      if value.lower() == 'true': value = True
      elif value.lower() == 'false': value = False
      else:
        try:
          value = float(value)
        except: pass

        state.config(name, value)


  def start(self, mach, planner):
    if self.cmd:
      self.log.info(self.cmd)

      if self.cmd[0] == '$': self._start_var_cmd(self.cmd)
      elif self.cmd[0] == '\\': mach.queue_command(self.cmd[1:])
      else:
        end_cb = lambda: mach.end(self)
        planner.load(end_cb, '<mdi>', self.cmd, False, self.with_limits)
        mach.resume()
        return True

    return False
