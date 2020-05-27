################################################################################
#                                                                              #
#                 This file is part of the Buildbotics firmware.               #
#                                                                              #
#        Copyright (c) 2015 - 2020, Buildbotics LLC, All rights reserved.      #
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

import lcd
import atexit


class LCDPage:
    def __init__(self, lcd, text = None):
        self.lcd = lcd
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


class LCD:
    def __init__(self, ctrl):
        self.ctrl = ctrl
        self.log = ctrl.log.get('LCD')

        self.addrs = self.ctrl.args.lcd_addr
        self.addr = self.addrs[0]
        self.addr_num = 0

        self.width = 20
        self.height = 4
        self.lcd = None
        self.timeout = None
        self.reset = False
        self.page = None
        self.pages = []
        self.current_page = 0
        self.screen = self.new_screen()
        self.set_message('Loading...')

        self._redraw(False)
        if not ctrl.args.demo: atexit.register(self.goodbye)


    def set_message(self, msg):
        try:
            self.load_page(LCDPage(self, msg))
            self._update()
        except IOError as e:
            self.log.warning('LCD communication failed: %s' % e)


    def new_screen(self):
        return [[' ' for y in range(self.height)] for x in range(self.width)]


    def new_page(self): return LCDPage(self)
    def add_page(self, page): self.pages.append(page)


    def add_new_page(self, page = None):
        if page is None: page = self.new_page()
        page.id = len(self.pages)
        self.add_page(page)
        return page


    def load_page(self, page):
        if self.page != page:
            if self.page is not None: self.page.deactivate()
            page.activate()
            self.page = page
            self.redraw = True
            self.update()


    def set_current_page(self, current_page):
        self.current_page = current_page % len(self.pages)
        self.load_page(self.pages[self.current_page])


    def page_up(self): pass
    def page_down(self): pass
    def page_right(self): self.set_current_page(self.current_page + 1)
    def page_left(self): self.set_current_page(self.current_page - 1)


    def update(self):
        if self.timeout is None:
            self.timeout = self.ctrl.ioloop.call_later(0.25, self._update)


    def _redraw(self, now = True):
        if now:
            self.redraw = True
            self.update()
        self.redraw_timer = self.ctrl.ioloop.call_later(5, self._redraw)


    def _update(self):
        self.timeout = None

        try:
            if self.lcd is None:
                self.lcd = lcd.LCD(self.ctrl.i2c, self.addr, self.height,
                                   self.width)

            if self.reset:
                self.lcd.reset()
                self.redraw = True
                self.reset = False

            cursorX, cursorY = -1, -1

            for y in range(self.height):
                for x in range(self.width):
                    c = self.page.data[x][y]

                    if self.redraw or self.screen[x][y] != c:
                        if cursorX != x or cursorY != y:
                            self.lcd.goto(x, y)
                            cursorX, cursorY = x, y

                        self.lcd.put_char(c)
                        cursorX += 1
                        self.screen[x][y] = c

            self.redraw = False

        except IOError as e:
            # Try next address
            self.addr_num += 1
            if len(self.addrs) <= self.addr_num: self.addr_num = 0
            self.addr = self.addrs[self.addr_num]
            self.lcd = None

            self.log.warning('LCD communication failed, ' +
                        'retrying on address 0x%02x: %s' % (self.addr, e))

            self.reset = True
            self.timeout = self.ctrl.ioloop.call_later(1, self._update)


    def goodbye(self, message = ''):
        if self.timeout:
            self.ctrl.ioloop.remove_timeout(self.timeout)
            self.timeout = None

        if self.redraw_timer:
            self.ctrl.ioloop.remove_timeout(self.redraw_timer)
            self.redraw_timer = None

        if self.lcd is not None: self.set_message(message)
