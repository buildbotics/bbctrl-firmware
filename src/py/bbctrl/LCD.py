import lcd
import atexit


class LCD:
    def __init__(self, port, addr):
        self.lcd = lcd.LCD(port, addr)
        self.splash()
        atexit.register(self.goodbye)


    def splash(self):
        self.lcd.clear()
        self.lcd.display(0, 'Buildbotics', lcd.JUSTIFY_CENTER)
        self.lcd.display(1, 'Controller', lcd.JUSTIFY_CENTER)
        self.lcd.display(3, '*Ready*', lcd.JUSTIFY_CENTER)


    def goodbye(self):
        self.lcd.clear()
        self.lcd.display(1, 'Goodbye', lcd.JUSTIFY_CENTER)
