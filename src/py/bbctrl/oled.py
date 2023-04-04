################################################################################
#                                                                              #
#                 This file is part of the Buildbotics firmware.               #
#                                                                              #
#        Copyright (c) 2015 - 2021, Buildbotics LLC, All rights reserved.      #
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

import time
from luma.core.interface.serial import i2c
from luma.oled.device import ssd1306
from luma.core.render import canvas
from PIL import Image

__all__ = ['OLED']

class OLED:

  def __init__(self,ctrl):
    self.log = ctrl.log.get('OLED')
    self.device = 0
    try:
      serial = i2c(port=1, address = 0x3C)
      self.device = ssd1306(i2c(port=1, address=0x3c), width = 128, height=64, rotate=0)
      self.device.contrast(1)
      self.log.info("oled initialization complete")
    except:
      self.log.info("oled not found")

  def write(self,message="hello",col=0,row=0):
    with canvas(self.device,dither=True) as draw:
      text_size = draw.textsize(message)
      draw.text((row,col * 8),message,fill='white')

  def writePage(self,page):
      with canvas(self.device,dither=True) as draw:
        j = 0
        while j < 4:
          s = ""
          i = 0
          while i < 20:
            s = s + str(page[i][j])
            i = i + 1
          text_size = draw.textsize(s)
          draw.text((0,5 + j*12),s,fill='white')
          j = j + 1
