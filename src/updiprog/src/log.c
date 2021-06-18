#include "log.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>


static uint8_t _level = LOG_LEVEL_WARNING;


void log_print(uint8_t level, const char *msg, ...) {
  va_list args;

  if (level < _level) return;

  switch (level) {
  case LOG_LEVEL_INFO:    printf("INFO: ");    break;
  case LOG_LEVEL_WARNING: printf("WARNING: "); break;
  case LOG_LEVEL_ERROR:   printf("ERROR: ");   break;
  }

  va_start(args, msg);
  vprintf(msg, args);
  va_end(args);
  printf("\n");
}


void log_verbose() {_level = LOG_LEVEL_INFO;}


void log_progress(uint16_t count, uint16_t total, const char *prefix) {
  uint8_t fill = (uint8_t)(PROGRESS_BAR_LENGTH * count / total);

  printf("\r%s [", prefix);

  for (uint8_t i = 0; i < PROGRESS_BAR_LENGTH; i++)
    putchar(i < fill ? '#' : ' ');

  printf("] %5.1f%% %u", (float)count / total * 100, count);

  // Print new line when complete
  if (total <= count) printf("\n");

  fflush(stdout);
}
