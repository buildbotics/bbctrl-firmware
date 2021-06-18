#include "updi.h"
#include "ihex.h"
#include "log.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/types.h>
#include <fcntl.h>


static device_t  *_dev = 0;
static bool _prog_mode = false;
static int         _fd = -1;


static uint16_t crc16(uint8_t byte, uint16_t crc) {
  for (int i = 0; i < 8; i++) {
    crc ^= byte & 1;
    crc = (crc & 1) ? (crc >> 1) ^ 0x8408 : (crc >> 1);
    byte = byte >> 1;
  }

  return crc;
}


static uint16_t crc16_block(const uint8_t *data, unsigned len, uint16_t crc) {
  for (unsigned i = 0; i < len; i++)
    crc = crc16(data[i], crc);

  return crc;
}


static speed_t _get_baud(uint32_t baud) {
  switch (baud) {
  case 300:     return B300;
  case 1200:    return B1200;
  case 2400:    return B2400;
  case 4800:    return B4800;
  case 9600:    return B9600;
  case 19200:   return B19200;
  case 38400:   return B38400;
  case 57600:   return B57600;
  case 115200:  return B115200;
  case 230400:  return B230400;
  case 460800:  return B460800;
  case 500000:  return B500000;
  case 576000:  return B576000;
  case 921600:  return B921600;
  case 1000000: return B1000000;
  case 1152000: return B1152000;
  case 1500000: return B1500000;
  case 2000000: return B2000000;
  case 2500000: return B2500000;
  case 3000000: return B3000000;
  case 3500000: return B3500000;
  case 4000000: return B4000000;
  default:      return -1;
  }
}


static void _com_close() {
  if (0 <= _fd) close(_fd);
  _fd = -1;
}


static bool _com_open(const char *port, uint32_t baud, bool parity,
                      bool two_stop) {
  _com_close();

  LOG_INFO("Opening %s at %u baud", port, baud);

  _fd = open(port, O_RDWR | O_NOCTTY);
  if (_fd < 0) return false;

  struct termios term;
  tcgetattr(_fd, &term); // Get current settings

  // Set baud rate
  speed_t s = _get_baud(baud);
  if (s == (speed_t)-1) return false;
  cfsetispeed(&term, s);
  cfsetospeed(&term, s);

  // Enter raw mode
  cfmakeraw(&term);

  // Parity
  if (parity) term.c_cflag |= PARENB;
  else term.c_cflag &= ~PARENB;

  // Stop bits
  if (two_stop) term.c_cflag |= CSTOPB;
  else term.c_cflag &= ~CSTOPB;

  term.c_cflag |= CREAD | CLOCAL; // Enable receiver, ignore modem control lines
  term.c_cc[VMIN]  = 0;           // Blocking read
  term.c_cc[VTIME] = 40;          // 4 second read timeout

  tcsetattr(_fd, TCSANOW, &term);
  tcflush(_fd, TCIFLUSH);

  return true;
}


static bool _com_receive(uint8_t *data, uint16_t len) {
  while (len) {
    int ret = read(_fd, data, len);
    if (ret <= 0) return false;
    len -= ret;
    data += ret;
  }

  return true;
}


static bool _com_send(uint8_t *data, uint8_t len) {
  write(_fd, data, len);
  return _com_receive(data, len);
}


/// Load data from Control/Status space
static uint8_t _load_cs(uint8_t address) {
  uint8_t response = 0;
  uint8_t buf[] = {UPDI_PHY_SYNC, UPDI_LDCS | (address & 0x0f)};

  LOG_INFO("Load CS[0x%02x]", address);
  _com_send(buf, sizeof(buf));
  _com_receive(&response, 1);

  return response;
}


/// Store a value to Control/Status space
static bool _store_cs(uint8_t address, uint8_t value) {
  uint8_t buf[] = {UPDI_PHY_SYNC, UPDI_STCS | (address & 0x0f), value};

  LOG_INFO("Store CS[0x%02x] = 0x%02x", address, value);
  return _com_send(buf, sizeof(buf));
}


/// Set the pointer location
static bool _store_ptr(uint16_t address) {
  uint8_t response;
  uint8_t buf[] = {UPDI_PHY_SYNC, UPDI_ST | UPDI_PTR_ADDRESS | UPDI_DATA_16,
    address, address >> 8};

  LOG_INFO("Store ptr");
  _com_send(buf, sizeof(buf));
  _com_receive(&response, 1);

  return response == UPDI_PHY_ACK;
}


/// Store value in repeat counter
static bool _store_repeat(uint16_t n) {
  uint8_t buf[] =
    {UPDI_PHY_SYNC, UPDI_REPEAT | UPDI_REPEAT_WORD, n - 1, (n - 1) >> 8};

  LOG_INFO("Store repeat %d", n);
  return _com_send(buf, sizeof(buf));
}


/// Load a single byte direct from a 16-bit address
static uint8_t _load_byte(uint16_t address) {
  uint8_t response = 0;
  uint8_t buf[] = {UPDI_PHY_SYNC, UPDI_LDS | UPDI_ADDRESS_16 | UPDI_DATA_8,
    address, address >> 8};

  LOG_INFO("Load from 0x%04x", address);
  _com_send(buf, sizeof(buf));
  _com_receive(&response, 1);

  return response;
}


/// Store a single byte value directly to a 16-bit address
static bool _store_byte(uint16_t address, uint8_t value) {
  uint8_t response;
  uint8_t buf[] = {UPDI_PHY_SYNC, UPDI_STS | UPDI_ADDRESS_16 | UPDI_DATA_8,
    address, address >> 8};

  LOG_INFO("Store byte [0x%04x] = 0x%02x", address, value);
  _com_send(buf, sizeof(buf));
  _com_receive(&response, 1);

  if (response != UPDI_PHY_ACK) return false;

  _com_send(&value, 1);
  _com_receive(&response, 1);

  return response == UPDI_PHY_ACK;
}


static bool _send_key(char *key, uint8_t size) {
  LOG_INFO("Writing key");

  if (strlen(key) != (unsigned)(8 << size)) {
    LOG_ERROR("Invalid key length");
    return false;
  }

  uint8_t buf[] = {UPDI_PHY_SYNC, UPDI_KEY | UPDI_KEY_KEY | size};
  _com_send(buf, sizeof(buf));

  uint8_t data[8];
  uint8_t i = 0;
  for (uint8_t n = strlen(key); 0 < n; n--)
    data[i++] = key[n - 1];

  _com_send(data, sizeof(data));

  return true;
}


/// Execute an NVM COMMAND on the NVM CTRL
static bool _execute(uint8_t command) {
  LOG_INFO("NVM %d executing", command);
  return _store_byte(UPDI_NVMCTRL_ADDR + UPDI_NVMCTRL_CTRLA, command);
}


/// Apply then release reset
static void _reset() {
  LOG_INFO("Reset");
  _store_cs(UPDI_ASI_RESET_REQ, UPDI_RESET_REQ_VALUE);
  _store_cs(UPDI_ASI_RESET_REQ, 0);
}


/// Check if the NVMPROG flag is set
static bool _in_prog_mode() {
  return _load_cs(UPDI_ASI_SYS_STATUS) & (1 << UPDI_ASI_SYS_STATUS_NVMPROG);
}


/// Check if the LOCKSTATUS flag is set
static bool _is_locked() {
  return _load_cs(UPDI_ASI_SYS_STATUS) & (1 << UPDI_ASI_SYS_STATUS_LOCKSTATUS);
}


/// Waits for the device to be unlocked
static bool _wait_unlocked() {
  uint16_t timeout = 1000;

  while (0 < timeout--) {
    if (!_is_locked()) return true;
    usleep(100); // 0.1ms
  }

  LOG_WARN("Timedout waiting for device unlock");
  return false;
}


/// Wait for the NVM controller to be ready
static bool _wait_flash_ready() {
  uint32_t timeout = 100000;

  LOG_INFO("Wait for flash ready");

  while (0 < timeout--) {
    uint8_t status = _load_byte(UPDI_NVMCTRL_ADDR + UPDI_NVMCTRL_STATUS);
    if (status & (1 << UPDI_NVM_STATUS_WRITE_ERROR)) {
      LOG_ERROR("NVM error");
      return false;
    }

    if (!(status & ((1 << UPDI_NVM_STATUS_EEPROM_BUSY) |
                    (1 << UPDI_NVM_STATUS_FLASH_BUSY))))
      return true;

    usleep(100); // 0.1ms
  }

  LOG_WARN("Timedout waiting for flash");

  return false;
}


/// Read a number of bytes from UPDI
static bool _read_data(uint16_t address, uint8_t *data, uint16_t len) {
  LOG_INFO("Reading %d bytes from 0x%04x", len, address);

  // Range check
  if (UPDI_MAX_REPEAT_SIZE < len) {
    LOG_ERROR("Can't read %d bytes in one go", len);
    return false;
  }

  // Set base address & repeat counter
  _store_ptr(address);
  _store_repeat(len >> 1);

  // Read words
  uint8_t buf[] = {UPDI_PHY_SYNC, UPDI_LD | UPDI_PTR_INC | UPDI_DATA_16};
  _com_send(buf, sizeof(buf));
  if (!_com_receive(data, len & ~1UL)) return false;

  // Last odd byte
  if (len & 1) data[len - 1] = _load_byte(address + len - 1);

  return true;
}


/// Write a number of bytes to UPDI
static bool _write_data(uint16_t address, uint8_t *data, uint16_t len) {
  LOG_INFO("Writing %d bytes to 0x%04x", len, address);

  if (1 < len) {
    // Range check
    if (UPDI_MAX_REPEAT_SIZE < (len >> 1)) {
      LOG_ERROR("Invalid length %d", len);
      return false;
    }

    // Set base address & repeat counter
    _store_ptr(address);
    _store_repeat(len >> 1);

    // Store words
    uint8_t buf[] =
      {UPDI_PHY_SYNC, UPDI_ST | UPDI_PTR_INC | UPDI_DATA_16, data[0], data[1]};
    _com_send(buf, sizeof(buf));

    uint8_t response = 0;
    _com_receive(&response, 1);
    if (response != UPDI_PHY_ACK) return false;

    for (uint16_t n = 2; n < len; n += 2) {
      _com_send(&data[n], sizeof(uint16_t));
      _com_receive(&response, 1);
      if (response != UPDI_PHY_ACK) return false;
    }
  }

  // Last odd byte
  if (len & 1) return _store_byte(address + len - 1, data[len - 1]);

  return true;
}


static bool _enter_prog_mode() {
  if (_prog_mode) return true;

  LOG_INFO("Entering NVM programming mode");

  // Check if NVM is enabled
  if (_in_prog_mode()) LOG_WARN("Already in NVM programming mode");
  else {
    _prog_mode = false;

    // Send key
    _send_key(UPDI_KEY_NVM, UPDI_KEY_64);

    // Check key status
    uint8_t status = _load_cs(UPDI_ASI_KEY_STATUS);
    LOG_INFO("Key status = 0x%02x", status);

    if (!(status & (1 << UPDI_ASI_KEY_STATUS_NVMPROG))) {
      LOG_WARN("Key not accepted");
      return false;
    }

    _reset();

    // Wait for unlock
    if (!_wait_unlocked()) {
      LOG_ERROR("Failed to enter NVM programming mode: device is locked");
      return false;
    }

    // Check for NVMPROG flag
    if (!_in_prog_mode()) {
      LOG_ERROR("Failed to enter NVM programming mode");
      return false;
    }

    LOG_INFO("Now in NVM programming mode");
  }

  _prog_mode = true;
  return true;
}


/// Erases then writes a page of data to NVM
static bool _write_page(uint16_t address, uint8_t *data, uint16_t len) {
  // Check that NVM controller is ready
  if (!_wait_flash_ready()) {
    LOG_WARN("Timedout waiting for flash ready before page buffer clear");
    return false;
  }

  // Clear the page buffer
  LOG_INFO("Clearing page buffer");
  _execute(UPDI_NVMCTRL_CTRLA_PAGE_BUFFER_CLR);

  // Wait for NVM controller to be ready
  if (!_wait_flash_ready()) {
    LOG_WARN("Timedout waiting for flash ready after page buffer clear");
    return false;
  }

  // Load the page buffer by writing directly to location
  _write_data(address, data, len);

  // Write the page to NVM, maybe erase first
  LOG_INFO("Committing page");
  _execute(UPDI_NVMCTRL_CTRLA_ERASE_WRITE_PAGE);

  // Wait for NVM controller to be ready again
  if (!_wait_flash_ready()) {
    LOG_WARN("Timedout waiting for flash ready after page write");
    return false;
  }

  return true;
}


static bool _break(const char *port) {
  LOG_INFO("Sending break");

  // Re-init at a lower baudrate
  // At 300 bauds, the break character will pull the line low for 30ms
  // Which is slightly above the recommended 24.6ms.  No parity, one stop bit
  if (!_com_open(port, 300, false, false)) return false;

  // Send two break characters, with 1 stop bit in between
  uint8_t buf[] = {UPDI_BREAK, UPDI_BREAK};
  _com_send(buf, sizeof(buf));
  _com_close();

  return true;
}


/// Init UPDI connection
bool updi_init(const char *port, uint32_t baud, device_t *dev) {
  if (!port) return false;

  LOG_MSG("Opening %s at %u baud", port, baud);

  if (access(port, R_OK | W_OK)) {
    LOG_ERROR("Port %s inaccessible", port);
    return false;
  }

  if (_get_baud(baud) == (speed_t)-1) {
    LOG_ERROR("Unsupported baud rate %d", baud);
    return false;
  }

  updi_close();

  uint32_t slow_baud = baud < 115200 ? baud : 115200;

  if (_break(port) && _com_open(port, slow_baud, true, true)) {
    // Disable collision detection, 2 cycle guard time, 16Mhz clock
    _store_cs(UPDI_CS_CTRLB,  UPDI_CTRLB_CCDETDIS);
    _store_cs(UPDI_CS_CTRLA,  UPDI_CTRLB_GTVAL_2);
    _store_cs(UPDI_ASI_CTRLA, UPDI_ASI_CTRLA_UPDICLKSEL_16MHZ);

    if (!_enter_prog_mode()) return false;

    // Check device signature
    uint32_t sig = updi_read_sig();
    if (dev && dev->sig != sig)
      LOG_WARN("Signature 0x%06x does not match device %s", sig, dev->name);

    if (!dev) {
      dev = devices_find_by_sig(sig);

      if (!dev) {
        LOG_ERROR("Device with signature 0x%06x unrecognized", sig);
        return false;
      }
    }

    _dev = dev;
    LOG_MSG("Device %s", dev->name);

    // Reopen at full speed
    if (baud == slow_baud || _com_open(port, baud, true, true)) {

      // Read status to check connection
      if (_load_cs(UPDI_CS_STATUSA)) {
        LOG_INFO("UPDI init OK");
        return true;
      }
    }
  }

  LOG_ERROR("UPDI init failed");
  _dev = 0;

  return false;
}


void updi_close() {
  LOG_INFO("UPDI closing");

  if (_prog_mode) {
    _wait_flash_ready(); // Wait for FLASH to finish any operations
    _reset();
    _store_cs(UPDI_CS_CTRLB, UPDI_CTRLB_UPDIDIS | UPDI_CTRLB_CCDETDIS);
    _prog_mode = false;
  }

  _com_close();
  _dev = 0;
}


uint32_t updi_read_sig() {
  return
    ((uint32_t)_load_byte(UPDI_SIGROW_ADDR + 0) << 16) +
    ((uint32_t)_load_byte(UPDI_SIGROW_ADDR + 1) <<  8) +
    ((uint32_t)_load_byte(UPDI_SIGROW_ADDR + 2) <<  0);
}


/// Does a chip erase using the NVM controller
/// Note that on locked devices this it not possible and the ERASE KEY has to
/// be used instead
bool updi_chip_erase() {
  LOG_MSG("Erasing chip");

  // Wait until NVM CTRL is ready to erase
  if (!_wait_flash_ready()) {
    LOG_WARN("Timedout waiting for flash ready before erase ");
    return false;
  }

  // Erase
  _execute(UPDI_NVMCTRL_CTRLA_CHIP_ERASE);

  // And wait for it
  if (!_wait_flash_ready()) {
    LOG_WARN("Timedout waiting for flash ready after erase");
    return false;
  }

  return true;
}


bool updi_chip_reset() {
  LOG_MSG("Resetting chip");
  _reset();
  return true;
}


static bool _rw_flash(bool read, uint16_t address, uint8_t *data,
                      uint16_t size) {
  const char *rwStr = read ? "Reading" : "Writing";

  // Find the number of pages
  uint16_t page_size = _dev->page_size;
  uint16_t pages     = size / page_size;
  if (size % page_size) pages++;

  // Fror each page
  uint16_t total = size;
  uint8_t errors = 0;
  for (uint16_t i = 0; i < pages;) {
    LOG_INFO("%s page at 0x%04x", rwStr, address);
    log_progress(total - size, total, rwStr);

    uint16_t bytes = size < page_size ? size : page_size;
    bool ret =
      (read ? _read_data : _write_page)(address, &data[i * page_size], bytes);

    if (!ret) {
      if (UPDI_MAX_ERRORS < ++errors) {
        LOG_MSG(""); // End progress
        return false;
      }

      LOG_WARN("Retrying page at 0x%04x", address);
      continue;
    } else errors = 0;

    address += page_size;
    size -= page_size;
    i++;
  }

  log_progress(total, total, rwStr);

  return true;
}


bool updi_read_flash(uint16_t address, uint8_t *data, uint16_t size) {
  return _rw_flash(true, address, data, size);
}


bool updi_write_flash(uint16_t address, uint8_t *data, uint16_t size) {
  return _rw_flash(false, address, data, size);
}


bool updi_write_fuse(uint8_t num, uint8_t value) {
  LOG_MSG("Writing fuse[%d] = 0x%02x", num, value);

  if (_dev->num_fuses <= num) {
    LOG_ERROR("Invalid fuse number %d", num);
    return false;
  }

  if (!_wait_flash_ready()) {
    LOG_ERROR("Flash not ready for fuse setting");
    return false;
  }

  uint16_t address = UPDI_FUSES_ADDR + num;
  _store_byte(UPDI_NVMCTRL_ADDR + UPDI_NVMCTRL_ADDRL, address);
  _store_byte(UPDI_NVMCTRL_ADDR + UPDI_NVMCTRL_ADDRH, address >> 8);
  _store_byte(UPDI_NVMCTRL_ADDR + UPDI_NVMCTRL_DATAL, value);
  _execute(UPDI_NVMCTRL_CTRLA_WRITE_FUSE);

  return true;
}


bool updi_write_fuses(const fuse_t *fuses, unsigned count) {
  for (unsigned i = 0; i < count; i++)
    if (!updi_write_fuse(fuses[i].num, fuses[i].value))
      return false;

  return true;
}


uint8_t updi_read_fuse(uint8_t num) {return _load_byte(UPDI_FUSES_ADDR + num);}


void updi_read_fuses() {
  LOG_MSG("Reading fuses:");

  for (uint8_t n = 0; n < _dev->num_fuses; n++) {
    uint8_t value = updi_read_fuse(n);
    LOG_MSG("  0x%02x: 0x%02x", n, value);
  }
}


bool updi_load_ihex(const char *filename, bool use_crc) {
  LOG_MSG("Writing from file: %s", filename);

  uint16_t address = _dev->flash_start;
  uint16_t size    = _dev->flash_size;
  bool res = false;

  uint8_t *buf = malloc(size);
  if (!buf) LOG_ERROR("Unable to allocate %d bytes\n", (int)size);

  else {
    if (use_crc) memset(buf, 0xff, size);

    FILE *fp = fopen(filename, "rt");
    if (!fp) LOG_ERROR("Unable to open file: %s", filename);

    else {
      if (use_crc) size -= 2; // Make space for CRC

      uint8_t err = ihex_read(fp, buf, size, &size);
      if (err != IHEX_ERROR_NONE)
        LOG_ERROR("Problem reading Hex file: %s", ihex_error_str(err));

      else {
        bool write = true;
        uint16_t crc = 0;

        if (use_crc) {
          crc = crc16_block(buf, _dev->flash_size, 0xffff);
          LOG_INFO("CRC 0x%04x Computed", crc);

          // Read CRC
          uint16_t flashEnd = _dev->flash_start + _dev->flash_size - 1;
          uint16_t flashCRC =
            ((uint16_t)_load_byte(flashEnd - 1) << 8) + _load_byte(flashEnd);
          LOG_INFO("CRC 0x%04x FLASH", flashCRC);

          if (crc == flashCRC) {
            LOG_MSG("CRCs match, nothing to do");
            write = false;
          }
        }

        if (!write) res = true;
        else {
          res = updi_write_flash(address, buf, size);

          if (use_crc && res) {
            buf[_dev->flash_size - 2] = crc >> 8;
            buf[_dev->flash_size - 1] = crc;

            uint16_t page_size = _dev->page_size;
            uint16_t addr = address + _dev->flash_size - page_size;
            _write_page(addr, &buf[_dev->flash_size - page_size], page_size);
          }
        }
      }

      fclose(fp);
    }

    free(buf);
  }

  return res;
}


bool updi_save_ihex(const char *filename) {
  LOG_MSG("Reading to file: %s", filename);

  uint16_t address = _dev->flash_start;
  uint16_t size    = _dev->flash_size;
  bool res = false;

  uint8_t *buf = malloc(size);
  if (!buf) LOG_ERROR("Unable to allocate %d bytes", (int)size);

  else {
    FILE *fp = fopen(filename, "w");
    if (!fp) LOG_ERROR("Unable to open file: %s", filename);

    else {
      if (!updi_read_flash(address, buf, size))
        LOG_ERROR("Reading from device failed");

      uint8_t err = ihex_write(fp, buf, size);
      if (err != IHEX_ERROR_NONE)
        LOG_ERROR("Problem writing Hex file: %s", ihex_error_str(err));
      else res = true;

      fclose(fp);
    }

    free(buf);
  }

  return res;
}
