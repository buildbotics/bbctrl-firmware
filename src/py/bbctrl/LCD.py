import lcd
import atexit
import logging


log = logging.getLogger('LCD')


class LCD:
    def __init__(self, ctrl):
        self.ctrl = ctrl
        self.force = False
        atexit.register(self.goodbye)
        self.connect()


    def connect(self):
        try:
            self.lcd = None
            self.lcd = lcd.LCD(self.ctrl.args.lcd_port, self.ctrl.args.lcd_addr)
            self.lcd.clear()
            self.lcd.display(1, 'Loading', lcd.JUSTIFY_CENTER)
            self.force = True

        except IOError as e:
            log.error('Connect failed, retrying: %s' % e)
            self.ctrl.ioloop.call_later(1, self.connect)


    def update(self, msg, force = False):
        def has(name): return self.force or force or name in msg

        if has('x') or has('c'):
            v = self.ctrl.avr.vars
            state = v.get('x', 'INIT')
            if 'c' in v and state == 'RUNNING': state = v['c']

            self.lcd.text('%-9s' % state, 0, 0)

        if has('xp'): self.lcd.text('% 10.4fX' % msg['xp'], 9, 0)
        if has('yp'): self.lcd.text('% 10.4fY' % msg['yp'], 9, 1)
        if has('zp'): self.lcd.text('% 10.4fZ' % msg['zp'], 9, 2)
        if has('ap'): self.lcd.text('% 10.4fA' % msg['ap'], 9, 3)
        if has('t'):  self.lcd.text('%2uT'     % msg['t'],  6, 1)
        if has('u'):  self.lcd.text('%s'       % msg['u'],  0, 1)
        if has('f'):  self.lcd.text('%8uF'     % msg['f'],  0, 2)
        if has('s'):  self.lcd.text('%8dS'     % msg['s'],  0, 3)

        self.force = False


    def goodbye(self):
        if self.lcd is None: return

        try:
            self.lcd.clear()
            self.lcd.display(1, 'Goodbye', lcd.JUSTIFY_CENTER)

        except IOError as e:
            log.error('I2C communication failed: %s' % e)
