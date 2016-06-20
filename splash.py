#!/usr/bin/env python3

import lcd

if __name__ == "__main__":
    screen = lcd.LCD(1, 0x27)

    screen.clear()
    screen.display(0, 'Buildbotics', lcd.JUSTIFY_CENTER)
    screen.display(1, 'Controller', lcd.JUSTIFY_CENTER)
    screen.display(3, 'Booting...', lcd.JUSTIFY_CENTER)
