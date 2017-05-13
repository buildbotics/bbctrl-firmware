#!/usr/bin/env python3

try:
    import smbus
except:
    import smbus2 as smbus

import time
import logging


log = logging.getLogger('LCD')


# Control flags
REG_SELECT_BIT          = 1 << 0
READ_BIT                = 1 << 1
ENABLE_BIT              = 1 << 2
BACKLIGHT_BIT           = 1 << 3

# Commands
LCD_CLEAR_DISPLAY       = 1 << 0
LCD_RETURN_HOME         = 1 << 1
LCD_ENTRY_MODE_SET      = 1 << 2
LCD_DISPLAY_CONTROL     = 1 << 3
LCD_CURSOR_SHIFT        = 1 << 4
LCD_FUNCTION_SET        = 1 << 5
LCD_SET_CGRAM_ADDR      = 1 << 6
LCD_SET_DDRAM_ADDR      = 1 << 7

# Entry Mode Set flags
LCD_ENTRY_SHIFT_DISPLAY = 1 << 0
LCD_ENTRY_SHIFT_INC     = 1 << 1
LCD_ENTRY_SHIFT_DEC     = 0 << 1

# Display Control flags
LCD_BLINK_ON            = 1 << 0
LCD_BLINK_OFF           = 0 << 0
LCD_CURSOR_ON           = 1 << 1
LCD_CURSOR_OFF          = 0 << 1
LCD_DISPLAY_ON          = 1 << 2
LCD_DISPLAY_OFF         = 0 << 2

# Cursor Shift flags
LCD_SHIFT_RIGHT         = 1 << 2
LCD_SHIFT_LEFT          = 0 << 2
LCD_SHIFT_DISPLAY       = 1 << 3
LCD_SHIFT_CURSOR        = 0 << 3

# Function Set flags
LCD_5x11_DOTS           = 1 << 2
LCD_5x8_DOTS            = 0 << 2
LCD_2_LINE              = 1 << 3
LCD_1_LINE              = 0 << 3
LCD_8_BIT_MODE          = 1 << 4
LCD_4_BIT_MODE          = 0 << 4

# Text justification flags
JUSTIFY_LEFT            = 0
JUSTIFY_RIGHT           = 1
JUSTIFY_CENTER          = 2



class LCD:
    def __init__(self, port, addr, height = 4, width = 20):
        self.addr = addr
        self.height = height
        self.width = width

        try:
            self.bus = smbus.SMBus(port)
        except FileNotFoundError as e:
            self.bus = None
            log.warning('Failed to open device: %s', e)

        self.backlight = True

        self.reset()


    def reset(self):
        time.sleep(0.050)
        self.write_nibble(3 << 4) # Home
        time.sleep(0.050)
        self.write_nibble(3 << 4) # Home
        time.sleep(0.050)
        self.write_nibble(3 << 4) # Home
        self.write_nibble(2 << 4) # 4-bit

        self.write(LCD_FUNCTION_SET | LCD_2_LINE | LCD_5x8_DOTS |
                   LCD_4_BIT_MODE)
        self.write(LCD_DISPLAY_CONTROL | LCD_DISPLAY_ON)
        self.write(LCD_ENTRY_MODE_SET | LCD_ENTRY_SHIFT_INC)


    def write_i2c(self, data):
        if self.bus is None: return

        if self.backlight: data |= BACKLIGHT_BIT

        self.bus.write_byte(self.addr, data)
        time.sleep(0.0001)


    # Write half of a command to LCD
    def write_nibble(self, data):
        self.write_i2c(data)

        # Strobe
        self.write_i2c(data | ENABLE_BIT)
        time.sleep(0.0005)

        self.write_i2c(data & ~ENABLE_BIT)
        time.sleep(0.0001)


    # Write an 8-bit command to LCD
    def write(self, cmd, flags = 0):
        self.write_nibble(flags | (cmd & 0xf0))
        self.write_nibble(flags | ((cmd << 4) & 0xf0))


    def set_cursor(self, on, blink):
        data = LCD_DISPLAY_CONTROL

        if on: data |= LCD_CURSOR_ON
        if blink: data |= LCD_BLINK_ON

        self.write(data)


    def set_backlight(self, enable):
        self.backlight = enable
        self.write_i2c(0)


    def program_char(self, addr, data):
        if addr < 0 or 8 <= addr: return

        self.write(LCD_SET_CGRAM_ADDR | (addr << 3))
        for x in data:
            self.write(x, REG_SELECT_BIT)


    def goto(self, x, y):
        if x < 0 or self.width <= x or y < 0 or self.height <= y: return
        self.write(LCD_SET_DDRAM_ADDR | (0, 64, 20, 84)[y] + int(x))


    def put_char(self, c):
        self.write(ord(c), REG_SELECT_BIT)


    def text(self, msg, x = None, y = None):
        if x is not None and y is not None: self.goto(x, y)

        for c in msg: self.put_char(c)


    def display(self, line, msg, justify = JUSTIFY_LEFT):
        if justify == JUSTIFY_RIGHT: x = self.width - len(msg)
        elif justify == JUSTIFY_CENTER: x = (self.width - len(msg)) / 2
        else: x = 0

        if x < 0: x = 0

        self.text(msg, x, line)


    def shift(self, count = 1, right = True, display = True):
        cmd = LCD_CURSOR_SHIFT
        if right: cmd |= LCD_SHIFT_RIGHT
        if display: cmd |= LCD_SHIFT_DISPLAY

        for i in range(count): self.write(cmd)


    # Clear LCD and move cursor home
    def clear(self):
        self.write(LCD_CLEAR_DISPLAY)
        self.write(LCD_RETURN_HOME)



if __name__ == "__main__":
    lcd = LCD(1, 0x27)

    lcd.clear()

    lcd.program_char(0, (0b11011,
                         0b11011,
                         0b00000,
                         0b01100,
                         0b01100,
                         0b00000,
                         0b11011,
                         0b11011))

    lcd.program_char(1, (0b11000,
                         0b01100,
                         0b00110,
                         0b00011,
                         0b00011,
                         0b00110,
                         0b01100,
                         0b11000))

    lcd.program_char(2, (0b00011,
                         0b00110,
                         0b01100,
                         0b11000,
                         0b11000,
                         0b01100,
                         0b00110,
                         0b00011))

    lcd.display(0, '\0' * lcd.width)
    lcd.display(1, 'Hello world!', JUSTIFY_CENTER)
    lcd.display(2, '\1\2' * (lcd.width / 2))
    lcd.display(3, '12345678901234567890')
