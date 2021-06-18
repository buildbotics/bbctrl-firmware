#pragma once

#include <stdint.h>


typedef struct {
  const char *name;
  uint32_t sig;
  uint16_t flash_start;
  uint16_t flash_size;
  uint16_t page_size;
  uint8_t  num_fuses;
} device_t;


extern device_t devices[];

device_t *devices_find(char *name);
device_t *devices_find_by_sig(uint32_t sig);
