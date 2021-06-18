#pragma once

#include "devices.h"

#include <stdint.h>
#include <stdbool.h>

// UPDI commands and control definitions
#define UPDI_BREAK        0x00

#define UPDI_LDS          0x00
#define UPDI_STS          0x40
#define UPDI_LD           0x20
#define UPDI_ST           0x60
#define UPDI_LDCS         0x80
#define UPDI_STCS         0xc0
#define UPDI_REPEAT       0xa0
#define UPDI_KEY          0xe0

#define UPDI_PTR          0x00
#define UPDI_PTR_INC      0x04
#define UPDI_PTR_ADDRESS  0x08

#define UPDI_ADDRESS_8    0x00
#define UPDI_ADDRESS_16   0x04

#define UPDI_DATA_8       0x00
#define UPDI_DATA_16      0x01

#define UPDI_KEY_SIB      0x04
#define UPDI_KEY_KEY      0x00

#define UPDI_KEY_64       0x00
#define UPDI_KEY_128      0x01

#define UPDI_SIB_8BYTES   UPDI_KEY_64
#define UPDI_SIB_16BYTES  UPDI_KEY_128

#define UPDI_REPEAT_BYTE  0x00
#define UPDI_REPEAT_WORD  0x01

#define UPDI_PHY_SYNC     0x55
#define UPDI_PHY_ACK      0x40

#define UPDI_MAX_REPEAT_SIZE 0xff

// CS and ASI register address map
#define UPDI_CS_STATUSA      0
#define UPDI_CS_STATUSB      1
#define UPDI_CS_CTRLA        2
#define UPDI_CS_CTRLB        3
#define UPDI_ASI_KEY_STATUS  7
#define UPDI_ASI_RESET_REQ   8
#define UPDI_ASI_CTRLA       9
#define UPDI_ASI_SYS_CTRLA  10
#define UPDI_ASI_SYS_STATUS 11
#define UPDI_ASI_CRC_STATUS 12

#define UPDI_CTRLA_IBDLY    (1 << 7)
#define UPDI_CTRLB_CCDETDIS (1 << 3)
#define UPDI_CTRLB_UPDIDIS  (1 << 2)

#define UPDI_CTRLB_GTVAL_128    0
#define UPDI_CTRLB_GTVAL_64     1
#define UPDI_CTRLB_GTVAL_32     2
#define UPDI_CTRLB_GTVAL_16     3
#define UPDI_CTRLB_GTVAL_8      4
#define UPDI_CTRLB_GTVAL_4      5
#define UPDI_CTRLB_GTVAL_2      6

#define UPDI_KEY_NVM       "NVMProg "
#define UPDI_KEY_CHIPERASE "NVMErase"
#define UPDI_KEY_USERROW   "NVMUs&te"

#define UPDI_ASI_STATUSA_REVID 4
#define UPDI_ASI_STATUSB_PESIG 0

#define UPDI_ASI_KEY_STATUS_CHIPERASE  3
#define UPDI_ASI_KEY_STATUS_NVMPROG    4
#define UPDI_ASI_KEY_STATUS_UROWWRITE  5

#define UPDI_ASI_SYS_STATUS_RSTSYS     5
#define UPDI_ASI_SYS_STATUS_INSLEEP    4
#define UPDI_ASI_SYS_STATUS_NVMPROG    3
#define UPDI_ASI_SYS_STATUS_UROWPROG   2
#define UPDI_ASI_SYS_STATUS_LOCKSTATUS 0

#define UPDI_RESET_REQ_VALUE    0x59

#define UPDI_ASI_CTRLA_UPDICLKSEL_16MHZ 1
#define UPDI_ASI_CTRLA_UPDICLKSEL_8MHZ  2
#define UPDI_ASI_CTRLA_UPDICLKSEL_4MHZ  3

// Flash controller
#define UPDI_NVMCTRL_CTRLA    0
#define UPDI_NVMCTRL_CTRLB    1
#define UPDI_NVMCTRL_STATUS   2
#define UPDI_NVMCTRL_INTCTRL  3
#define UPDI_NVMCTRL_INTFLAGS 4
#define UPDI_NVMCTRL_DATAL    6
#define UPDI_NVMCTRL_DATAH    7
#define UPDI_NVMCTRL_ADDRL    8
#define UPDI_NVMCTRL_ADDRH    9

// CTRLA
#define UPDI_NVMCTRL_CTRLA_NOP              0
#define UPDI_NVMCTRL_CTRLA_WRITE_PAGE       1
#define UPDI_NVMCTRL_CTRLA_ERASE_PAGE       2
#define UPDI_NVMCTRL_CTRLA_ERASE_WRITE_PAGE 3
#define UPDI_NVMCTRL_CTRLA_PAGE_BUFFER_CLR  4
#define UPDI_NVMCTRL_CTRLA_CHIP_ERASE       5
#define UPDI_NVMCTRL_CTRLA_ERASE_EEPROM     6
#define UPDI_NVMCTRL_CTRLA_WRITE_FUSE       7

#define UPDI_NVM_STATUS_WRITE_ERROR 2
#define UPDI_NVM_STATUS_EEPROM_BUSY 1
#define UPDI_NVM_STATUS_FLASH_BUSY  0

#define UPDI_SIB_LENGTH 16
#define UPDI_MAX_ERRORS 3

#define UPDI_NVMCTRL_ADDR 0x1000
#define UPDI_SIGROW_ADDR  0x1100
#define UPDI_FUSES_ADDR   0x1280

typedef struct {
  uint8_t num;
  uint8_t value;
} fuse_t;


bool updi_init(const char *port, uint32_t baud, device_t *dev);
void updi_close();
uint32_t updi_read_sig();
bool updi_chip_erase();
bool updi_chip_reset();
bool updi_read_flash(uint16_t address, uint8_t *data, uint16_t size);
bool updi_write_flash(uint16_t address, uint8_t *data, uint16_t size);
bool updi_write_fuse(uint8_t fuse, uint8_t value);
bool updi_write_fuses(const fuse_t *fuses, unsigned count);
uint8_t updi_read_fuse(uint8_t fuse);
void updi_read_fuses();
bool updi_load_ihex(const char *filename, bool use_crc);
bool updi_save_ihex(const char *filename);
