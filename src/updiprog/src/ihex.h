#pragma once

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>


#define IHEX_LINE_LENGTH    16
#define IHEX_MIN_STRING     11

#define IHEX_OFFS_LEN       1
#define IHEX_OFFS_ADDR      3
#define IHEX_OFFS_TYPE      7
#define IHEX_OFFS_DATA      9

#define IHEX_START          ":"
#define IHEX_NEWLINE        "\n"
#define IHEX_ENDFILE        ":00000001FF"

enum {
  IHEX_DATA_RECORD,
  IHEX_END_OF_FILE_RECORD,
  IHEX_EXTENDED_SEGMENT_ADDRESS_RECORD,
  IHEX_START_SEGMENT_ADDRESS_RECORD,
  IHEX_EXTENDED_LINEAR_ADDRESS_RECORD,
  IHEX_START_LINEAR_ADDRESS_RECORD
};

enum {
  IHEX_ERROR_NONE,
  IHEX_ERROR_FILE,
  IHEX_ERROR_SIZE,
  IHEX_ERROR_FMT,
  IHEX_ERROR_CRC
};


uint8_t ihex_write(FILE *f, uint8_t *data, uint16_t len);
uint8_t ihex_read(FILE *f, uint8_t *data, uint16_t maxlen, uint16_t *max_addr);
const char *ihex_error_str(uint8_t err);
