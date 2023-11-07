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

__all__ = ['LCDPage']


class LCDPage:
  def __init__(self, lcd, text = None):
    self.lcd  = lcd
    self.data = lcd.new_screen()

    if text is not None:
      self.text(text, (lcd.width - len(text)) // 2, 1)


  def activate(self): pass
  def deactivate(self): pass


  def put(self, c, x, y):
    y += x // self.lcd.width
    x %= self.lcd.width
    y %= self.lcd.height

    if self.data[x][y] != c:
      self.data[x][y] = c
      if self == self.lcd.page: self.lcd.update()


  def text(self, s, x, y):
    for c in s:
      self.put(c, x, y)
      x += 1


  def clear(self):
    self.data = self.lcd.new_screen()
    self.lcd.redraw = True


  def shift_left(self): pass
  def shift_right(self): pass
  def shift_up(self): pass
  def shift_down(self): pass
