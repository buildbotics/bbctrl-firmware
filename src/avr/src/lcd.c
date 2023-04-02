/******************************************************************************\

                  This file is part of the Buildbotics firmware.

         Copyright (c) 2015 - 2021, Buildbotics LLC, All rights reserved.

          This Source describes Open Hardware and is licensed under the
                                  CERN-OHL-S v2.

          You may redistribute and modify this Source and make products
     using it under the terms of the CERN-OHL-S v2 (https:/cern.ch/cern-ohl).
            This Source is distributed WITHOUT ANY EXPRESS OR IMPLIED
     WARRANTY, INCLUDING OF MERCHANTABILITY, SATISFACTORY QUALITY AND FITNESS
      FOR A PARTICULAR PURPOSE. Please see the CERN-OHL-S v2 for applicable
                                   conditions.

                 Source location: https://github.com/buildbotics

       As per CERN-OHL-S v2 section 4, should You produce hardware based on
     these sources, You must maintain the Source Location clearly visible on
     the external case of the CNC Controller or other product you make using
                                   this Source.

                 For more information, email info@buildbotics.com

\******************************************************************************/

#include "lcd.h"
#include "oled.h"
#include "rtc.h"
#include "hardware.h"
#include "command.h"

#include <avr/io.h>
#include <avr/wdt.h>
#include <util/delay.h>

#include <stdbool.h>


void lcd_init(uint8_t addr) {
  // Enable I2C master
  TWIC.MASTER.BAUD = F_CPU / 2 / 100000 - 5; // 100 KHz
  TWIC.MASTER.CTRLA = TWI_MASTER_ENABLE_bm;
  TWIC.MASTER.CTRLB = TWI_MASTER_TIMEOUT_DISABLED_gc;
  TWIC.MASTER.STATUS = TWI_MASTER_BUSSTATE_IDLE_gc;

  _delay_ms(50);
  lcd_nibble(addr, 3 << 4); // Home
  _delay_ms(50);
  lcd_nibble(addr, 3 << 4); // Home
  _delay_ms(50);
  lcd_nibble(addr, 3 << 4); // Home
  lcd_nibble(addr, 2 << 4); // 4-bit

  lcd_write(addr,
            LCD_FUNCTION_SET | LCD_2_LINE | LCD_5x8_DOTS | LCD_4_BIT_MODE, 0);
  lcd_write(addr, LCD_DISPLAY_CONTROL | LCD_DISPLAY_ON, 0);
  lcd_write(addr, LCD_ENTRY_MODE_SET | LCD_ENTRY_SHIFT_INC, 0);

  lcd_write(addr, LCD_CLEAR_DISPLAY, 0);
  lcd_write(addr, LCD_RETURN_HOME, 0);
}


static void _master_wait() {
#ifdef __AVR__
  while (!(TWIC.MASTER.STATUS & TWI_MASTER_WIF_bm)) continue;
#endif
}


static void _write_i2c(uint8_t addr, uint8_t data) {
  data |= BACKLIGHT_BIT;

  TWIC.MASTER.ADDR = addr << 1;
  _master_wait();

  TWIC.MASTER.DATA = data;
  _master_wait();

  TWIC.MASTER.CTRLC = TWI_MASTER_CMD_STOP_gc;

  _delay_us(100);
}


void lcd_nibble(uint8_t addr, uint8_t data) {
  _write_i2c(addr, data);
  _write_i2c(addr, data | ENABLE_BIT);
  _delay_us(500);
  _write_i2c(addr, data & ~ENABLE_BIT);
  _delay_us(100);
}


void lcd_write(uint8_t addr, uint8_t cmd, uint8_t flags) {
  lcd_nibble(addr, flags | (cmd & 0xf0));
  lcd_nibble(addr, flags | ((cmd << 4) & 0xf0));
}


void lcd_goto(uint8_t addr, uint8_t x, uint8_t y) {
  static uint8_t row[] = {0, 64, 20, 84};
  lcd_write(addr, LCD_SET_DDRAM_ADDR | (row[y] + x), 0);
}


void lcd_putchar(uint8_t addr, uint8_t c) {
  lcd_write(addr, c, REG_SELECT_BIT);
}


void lcd_pgmstr(uint8_t addr, const char *s) {
  while (true) {
    char c = pgm_read_byte(s++);
    if (!c) break;
    lcd_putchar(addr, c);
  }
}


void _splash(uint8_t addr) {
  lcd_init(addr);
  lcd_goto(addr, 0, 0);
  lcd_pgmstr(addr, PSTR("Controller booting"));
  lcd_goto(addr, 0, 1);
  lcd_pgmstr(addr, PSTR("Please wait..."));
  lcd_goto(addr, 0, 3);
  lcd_pgmstr(addr, PSTR("(c) Buildbotics LLC"));
}


void lcd_splash() {
  wdt_disable();
  oled_splash();
  _splash(0x27);
  _splash(0x3f);
  wdt_enable(WDTO_250MS);
}


void lcd_rtc_callback() {
  // Display the splash if we haven't gotten any commands in 1sec since boot
  if (!command_is_active() && rtc_get_time() == 1000)
    lcd_splash();
}
