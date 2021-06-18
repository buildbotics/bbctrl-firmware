#include "updi.h"
#include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>
#include <unistd.h>


#define SW_VERSION "1.1.0"
#define MAX_FUSES     32


typedef struct {
  uint32_t   baudrate;
  device_t   *device;
  bool       erase;
  bool       reset;
  bool       rd_fuses;
  bool       use_crc;
  const char *port;
  const char *write_file;
  const char *read_file;
  uint8_t    num_fuses;
  fuse_t     fuses[MAX_FUSES];
} args_t;


static void help() {
  printf(
    "UPDI programmer version " SW_VERSION "\n"
    "\n"
    "License: BSD 2-Clause\n"
    "Authors: Joe Coffland & Alex Kiselev\n"
    "Copyright 2021, Buildbotics LLC\n"
    "Copyright 2018, Alex Kiselev\n"
    "\n"
    "Usage: updiprog [OPTIONS]\n"
    "\n"
    "OPTIONS:\n"
    "  -b BAUDRATE    - Serial port speed (default=115200)\n"
    "  -d DEVICE      - Target device (optional)\n"
    "  -c PORT        - Serial port (e.g. /dev/ttyUSB0)\n"
    "  -e             - Chip erase\n"
    "  -R             - Chip reset\n"
    "  -F             - Read all fuses\n"
    "  -f FUSE VALUE  - Write fuse\n"
    "  -r FILE.HEX    - Hex file to read FLASH into\n"
    "  -w FILE.HEX    - Hex file to write to FLASH\n"
    "  -x             - Enable CRC checks\n"
    "  -v             - Verbose logging\n"
    "  -h             - Show this help screen and exit\n"
    "\n"
    "Supported devices:\n  ");

  for (int i = 0; devices[i].name; i++) {
    printf("%-10s", devices[i].name);
    if ((i + 1) % 5 == 0) printf("\n  ");
  }

  printf("\n");
}


static int parse_num(const char *s) {
  char *end = 0;
  long x = strtol(s, &end, 0);
  if (!end || *end) return -1;
  return x;
}


static bool parse_args(int argc, char *argv[], args_t *args) {
  for (uint8_t i = 1; i < argc; i++) {
    if (argv[i][0] == '-' && argv[i][1] && !argv[i][2]) {
      switch (argv[i][1]) {
      case 'b':
        if (++i == argc) {
          LOG_ERROR("Missing baudrate parameter");
          return false;
        }
        if (sscanf(argv[i], "%u", &args->baudrate) != 1) {
          LOG_ERROR("Invalid baudrate: %s", argv[i]);
          return false;
        }
        break;

      case 'c':
        if (++i == argc) {
          LOG_ERROR("Missing port parameter");
          return false;
        }
        args->port = argv[i];
        break;

      case 'd':
        if (++i == argc) {
          LOG_ERROR("Missing device parameter");
          return false;
        }
        if (!(args->device = devices_find(argv[i]))) {
          LOG_ERROR("Wrong or unsupported device type: %s", argv[i]);
          return false;
        }
        break;

      case 'e': args->erase    = true; break;
      case 'R': args->reset    = true; break;
      case 'F': args->rd_fuses = true; break;

      case 'f':
        if (args->num_fuses == MAX_FUSES) {
          LOG_ERROR("Too many fuses");
          return false;
        }

        if (++i == argc) {
          LOG_ERROR("Missing fuse number");
          return false;
        }
        int num = parse_num(argv[i]);
        if (num < 0) {
          LOG_ERROR("Invalid fuse number %s", argv[i]);
          return false;
        }

        if (++i == argc) {
          LOG_ERROR("Missing fuse value");
          return false;
        }
        int value = parse_num(argv[i]);
        if (value < 0 || 255 < value) {
          LOG_ERROR("Invalid fuse value %s", argv[i]);
          return false;
        }

        fuse_t *fuse = &args->fuses[args->num_fuses++];
        fuse->num    = num;
        fuse->value  = value;
        break;

      case 'x': args->use_crc = true; break;

      case 'r':
        if (++i == argc) {
          LOG_ERROR("Missing HEX read file");
          return false;
        }
        args->read_file = argv[i];
        break;

      case 'w':
        if (++i == argc) {
          LOG_ERROR("Missing HEX write file");
          return false;
        }
        args->write_file = argv[i];
        break;

      case 'v': log_verbose(); break;
      case 'h': help(); exit(0);

      default:
        LOG_ERROR("Invalid argument: %s", argv[i]);
        return false;
      }

    } else {
      LOG_ERROR("Invalid argument: %s", argv[i]);
      return false;
    }
  }

  if (strlen(args->port) == 0) {
    LOG_ERROR("Port name missing");
    return false;
  }

  return true;
}


bool run(args_t *args) {
  if (args->erase && !updi_chip_erase()) return false;
  if (args->reset && !updi_chip_reset()) return false;
  if (args->num_fuses && !updi_write_fuses(args->fuses, args->num_fuses))
    return false;
  if (args->rd_fuses) updi_read_fuses();
  if (args->write_file && !updi_load_ihex(args->write_file, args->use_crc))
    return false;
  if (args->read_file && !updi_save_ihex(args->read_file)) return false;

  return true;
}


int main(int argc, char *argv[]) {
  args_t args = {.baudrate = 115200};

  if (!parse_args(argc, argv, &args)) return 1;
  if (!updi_init(args.port, args.baudrate, args.device)) return 1;

  bool ok = run(&args);
  updi_close();

  return ok ? 0 : 1;
}
