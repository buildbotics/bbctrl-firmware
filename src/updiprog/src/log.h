#pragma once

#include <stdint.h>
#include <stdbool.h>


#define PROGRESS_BAR_LENGTH 20

enum {
  LOG_LEVEL_INFO,
  LOG_LEVEL_WARNING,
  LOG_LEVEL_ERROR,
  LOG_LEVEL_MSG,
  LOG_LEVEL_LAST
};


void log_print(uint8_t level, const char *msg, ...);
void log_verbose();
void log_progress(uint16_t count, uint16_t total, const char *prefix);

#define LOG_INFO(...)  log_print(LOG_LEVEL_INFO, __VA_ARGS__)
#define LOG_WARN(...)  log_print(LOG_LEVEL_WARNING, __VA_ARGS__)
#define LOG_ERROR(...) log_print(LOG_LEVEL_ERROR, __VA_ARGS__)
#define LOG_MSG(...)   log_print(LOG_LEVEL_MSG, __VA_ARGS__)
