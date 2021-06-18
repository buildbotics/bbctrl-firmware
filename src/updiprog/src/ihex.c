#include "ihex.h"

#include <stdio.h>
#include <ctype.h>
#include <string.h>


#define IHEX_DIGIT(n) ((char)((n) + (((n) < 10) ? '0' : ('A' - 10))))


static char *_add_byte(uint8_t byte, uint8_t *crc) {
  static char res[3];

  *crc += byte;
  uint8_t n = (byte & 0xF0U) >> 4; // high nybble
  res[0] = IHEX_DIGIT(n);
  n = byte & 0x0FU; // low nybble
  res[1] = IHEX_DIGIT(n);
  res[2] = 0;

  return res;
}


uint8_t ihex_write(FILE *f, uint8_t *data, uint16_t len) {
  uint8_t width;
  char str[128];
  uint8_t crc = 0;

  for (unsigned i = 0; i < len; i += IHEX_LINE_LENGTH) {
    strcpy(str, IHEX_START);

    // write length
    if (len - i >= IHEX_LINE_LENGTH) width = IHEX_LINE_LENGTH;
    else width = (uint8_t)(len - i);
    strcat(str, _add_byte(width, &crc));

    // write address
    strcat(str, _add_byte((uint8_t)(i >> 8), &crc));
    strcat(str, _add_byte((uint8_t)i, &crc));

    // write type (data)
    strcat(str, _add_byte(IHEX_DATA_RECORD, &crc));
    for (unsigned x = 0; x < width; x++)
      strcat(str, _add_byte(*data++, &crc));

    crc = (uint8_t)(0x100 - crc);
    strcat(str, _add_byte(crc, &crc));
    strcat(str, IHEX_NEWLINE);
    fwrite(str, strlen(str), 1, f);
    crc = 0;
  }

  // Write end
  fwrite(IHEX_ENDFILE, strlen(IHEX_ENDFILE), 1, f);
  fwrite(IHEX_NEWLINE, strlen(IHEX_NEWLINE), 1, f);

  return IHEX_ERROR_NONE;
}


static uint8_t _get_nibble(char c) {
  if ('0' <= c && c <= '9') return (uint8_t)(c - '0');
  if ('A' <= c && c <= 'F') return (uint8_t)(c - 'A' + 10);
  if ('a' <= c && c <= 'f') return (uint8_t)(c - 'a' + 10);
  return 0;
}


static uint8_t _get_byte(char *data) {
  uint8_t res = _get_nibble(*data++) << 4;
  res += _get_nibble(*data);
  return res;
}


uint8_t ihex_read(FILE *f, uint8_t *data, uint16_t maxlen, uint16_t *max_addr) {
  uint16_t segment = 0;

  while (!feof(f)) {
    char str[128];
    if (!fgets(str, sizeof(str), f)) return IHEX_ERROR_FILE;

    // trim whitespace
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    if (strlen(str) < IHEX_MIN_STRING) return IHEX_ERROR_FMT;

    uint8_t len = _get_byte(&str[IHEX_OFFS_LEN]);
    uint16_t addr = (_get_byte(&str[IHEX_OFFS_ADDR]) << 8) +
      _get_byte(&str[IHEX_OFFS_ADDR + 2]);
    if (maxlen <= addr + segment) return IHEX_ERROR_SIZE;

    uint8_t type = _get_byte(&str[IHEX_OFFS_TYPE]);
    if (len * 2 + IHEX_MIN_STRING != strlen(str)) return IHEX_ERROR_FMT;

    switch (type) {
      case IHEX_DATA_RECORD:
        for (uint8_t i = 0; i < len; i++) {
          uint8_t byte = _get_byte(&str[IHEX_OFFS_DATA + i * 2]);
          if (byte != 0xFF) *max_addr = addr + segment + i + 1;
          data[addr + segment + i] = byte;
        }
        break;

      case IHEX_END_OF_FILE_RECORD: return IHEX_ERROR_NONE;

      case IHEX_EXTENDED_SEGMENT_ADDRESS_RECORD:
        segment = ((_get_byte(&str[IHEX_OFFS_DATA]) << 8) +
                   _get_byte(&str[IHEX_OFFS_DATA + 2])) << 4;
        break;

      case IHEX_START_SEGMENT_ADDRESS_RECORD:   break;
      case IHEX_EXTENDED_LINEAR_ADDRESS_RECORD: break;
      case IHEX_START_LINEAR_ADDRESS_RECORD:    break;
      default: return IHEX_ERROR_FMT;
    }
  }

  return IHEX_ERROR_NONE;
}


const char *ihex_error_str(uint8_t err) {
  switch (err) {
  case IHEX_ERROR_NONE: return "None";
  case IHEX_ERROR_FILE: return "I/O error";
  case IHEX_ERROR_SIZE: return "Size too large";
  case IHEX_ERROR_FMT:  return "Invalid format";
  case IHEX_ERROR_CRC:  return "CRC check failed";
  }

  return "Unknown";
}
