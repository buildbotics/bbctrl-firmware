import lcd
import atexit


class LCD:
    def __init__(self, ctrl):
        self.ctrl = ctrl

        self.lcd = lcd.LCD(ctrl.args.lcd_port, ctrl.args.lcd_addr)
        atexit.register(self.goodbye)


    def update(self, msg, force = False):
        def has(name): return force or name in msg

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


    def goodbye(self):
        self.lcd.clear()
        self.lcd.display(1, 'Goodbye', lcd.JUSTIFY_CENTER)
