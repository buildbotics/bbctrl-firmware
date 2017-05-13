import lcd
import atexit
import logging
import tornado.ioloop


log = logging.getLogger('LCD')


class LCD:
    def __init__(self, ctrl):
        self.ctrl = ctrl

        self.width = 20
        self.height = 4
        self.lcd = None
        self.timeout = None
        self.clear_next_write = False

        self.clear()
        self.text('Loading', 6, 1)
        self.clear_next_write = True
        self.update_screen()

        # Redraw screen every 5 seconds
        self.redraw_timer = tornado.ioloop.PeriodicCallback(self._redraw, 5000,
                                                            self.ctrl.ioloop)
        self.redraw_timer.start()

        atexit.register(self.goodbye)


    def clear(self):
        self.screen = [[[' ', False] for x in range(self.width)]
                       for y in range(self.height)]
        self.redraw = True


    def _trigger_update(self):
        if self.timeout is None:
            self.timeout = self.ctrl.ioloop.call_later(0.25, self.update_screen)


    def _redraw(self):
        self.redraw = True
        self._trigger_update()


    def put(self, c, x, y):
        if self.clear_next_write:
            self.clear_next_write = False
            self.clear()

        y += x // self.width
        x %= self.width
        y %= self.height

        if self.screen[y][x][0] != c:
            self.screen[y][x] = [c, True]
            self._trigger_update()


    def text(self, s, x, y):
        for c in s:
            self.put(c, x, y)
            x += 1


    def update_screen(self):
        self.timeout = None

        try:
            if self.lcd is None:
                self.lcd = lcd.LCD(self.ctrl.args.lcd_port,
                                   self.ctrl.args.lcd_addr,
                                   self.height, self.width)

            cursorX, cursorY = -1, -1

            for y in range(self.height):
                for x in range(self.width):
                    cell = self.screen[y][x]

                    if self.redraw or cell[1]:
                        if cursorX != x or cursorY != y:
                            self.lcd.goto(x, y)
                            cursorX, cursorY = x, y

                        self.lcd.put_char(cell[0])
                        cursorX += 1
                        cell[1] = False

            self.redraw = False

        except IOError as e:
            log.error('LCD communication failed, retrying: %s' % e)
            self.redraw = True
            self.timeout = self.ctrl.ioloop.call_later(1, self.update_screen)


    def update(self, msg):
        if 'x' in msg or 'c' in msg:
            v = self.ctrl.avr.vars
            state = v.get('x', 'INIT')
            if 'c' in v and state == 'RUNNING': state = v['c']

            self.text('%-9s' % state, 0, 0)

        if 'xp' in msg: self.text('% 10.4fX' % msg['xp'], 9, 0)
        if 'yp' in msg: self.text('% 10.4fY' % msg['yp'], 9, 1)
        if 'zp' in msg: self.text('% 10.4fZ' % msg['zp'], 9, 2)
        if 'ap' in msg: self.text('% 10.4fA' % msg['ap'], 9, 3)
        if 't' in msg:  self.text('%2uT'     % msg['t'],  6, 1)
        if 'u' in msg:  self.text('%s'       % msg['u'],  0, 1)
        if 'f' in msg:  self.text('%8uF'     % msg['f'],  0, 2)
        if 's' in msg:  self.text('%8dS'     % msg['s'],  0, 3)


    def goodbye(self):
        if self.timeout:
            self.ctrl.ioloop.remove_timeout(self.timeout)
            self.timeout = None

        if self.redraw_timer:
            self.redraw_timer.stop()
            self.redraw_timer = None

        if self.lcd is not None:
            try:
                self.lcd.clear()
                self.lcd.display(1, 'Goodbye', lcd.JUSTIFY_CENTER)

            except IOError as e:
                log.error('LCD communication failed: %s' % e)
