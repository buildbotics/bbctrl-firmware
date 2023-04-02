#include <util/delay.h>
#include <stdint.h>
#include <string.h>
#include "oled.h"
#include "i2c.h"
#include "lcd.h"

static void _master_wait() {
#ifdef __AVR__
  while (!(TWIC.MASTER.STATUS & TWI_MASTER_WIF_bm)) continue;
#endif
}

void _sendByte(uint8_t ch){
  TWIC.MASTER.DATA = ch;
  _master_wait();

}

void _sendCommand(uint8_t cmd){
    _sendByte(0x00); // next byte is command
    _sendByte(cmd);
}

void _gotorowcol(int row = 0, int col = 0) {
  TWIC.MASTER.ADDR = OLED_ADDR << 1;
  _master_wait();
  _sendByte(0x00);
  _sendByte(0x21); // set column address
  _sendByte(col);
  _sendByte(127);
  _sendByte(0x22); // set page address
  _sendByte(row);
  _sendByte(7);
  TWIC.MASTER.CTRLC = TWI_MASTER_CMD_STOP_gc;
  _delay_us(100);
}

void _writeStringAt(char * s, int row = 0, int col = 0) {
  unsigned int i,j,line;
  _gotorowcol(row,col);
  TWIC.MASTER.ADDR = OLED_ADDR << 1;
  _master_wait();
  _sendByte(0x40);
  for (i = 0; i < strlen(s); i++) {
    line = s[i] - 0x20;
    for (j=0; j < 5; j++) {
      _sendByte(fontTable[line][j]);
    }
  }
  TWIC.MASTER.CTRLC = TWI_MASTER_CMD_STOP_gc;
  _delay_us(100);
  _gotorowcol(0,0);
}

void oled_clear() {
  int i;
  _gotorowcol(0,0);  
  TWIC.MASTER.ADDR = OLED_ADDR << 1;
  _master_wait();
  _sendByte(0x40);
  for (i = 0; i < 1024; i++) {
    _sendByte(0x00);
  }
  TWIC.MASTER.CTRLC = TWI_MASTER_CMD_STOP_gc;
  _delay_us(300);
  _gotorowcol(0,0);
}  

void oled_init() {
  // Enable I2C master
  TWIC.MASTER.BAUD = F_CPU / 2 / 100000 - 5; // 100 KHz
  TWIC.MASTER.CTRLA = TWI_MASTER_ENABLE_bm;
  TWIC.MASTER.CTRLB = TWI_MASTER_TIMEOUT_DISABLED_gc;
  TWIC.MASTER.STATUS = TWI_MASTER_BUSSTATE_IDLE_gc;
  
  _delay_ms(50);
  TWIC.MASTER.ADDR = OLED_ADDR << 1;
  _master_wait();
  _sendByte(0x00);  
  _sendByte(0xAE); // display off
  _sendByte(0xD5); // set display clock division ratio
  _sendByte(0x80);
  _sendByte(0xA8); // set multiplex ratio
  _sendByte(0x3F);
  _sendByte(0xD3); // set display offset
  _sendByte(0x00);
  _sendByte(0x40 | 0); // set start line to zero
  _sendByte(0x8D); // charge pump
  _sendByte(0x14);
  _sendByte(0x20); // memory access mode
  _sendByte(0x00);
  _sendByte(0xA0 | 0x01); // set segment remap
  _sendByte(0xC8); // set scan dir to decreasing
  _sendByte(0xDA); // set com pins
  _sendByte(0x12);
  _sendByte(0x81); // set contrast control
  _sendByte(0x80);
  _sendByte(0xD9); // set precharge period
  _sendByte(0xF1);
  _sendByte(0xDB); // set vcom deselect
  _sendByte(0x20);
  _sendByte(0xA4); // display all on resume
  _sendByte(0xA6); // normal display
  _sendByte(0xAF); // display on 
  _delay_us(300);
  oled_clear();
};

void oled_splash() {
  _delay_ms(1000);
  oled_init();
  _writeStringAt((char *) "Controller booting",1,5);
  _writeStringAt((char *) "Please wait...",3,5);
  _writeStringAt((char *) "(c) Buildbotics LLC",4,5);
  _delay_us(100);
};
