# updiprog

This is UPDI programming software for AVR microcontrollers.  It was based on the
code at https://github.com/Polarisru/updiprog but has undergone a lot of
restructuring and has speed and reliability improvements and additional features.
However, this version does not work on Windows.

```
                        Vcc                     Vcc
                        +-+                     +-+
                         |                       |
 +---------------------+ |                       | +--------------------+
 | Serial port         +-+                       +-+  AVR device        |
 |                     |      +----------+         |                    |
 |                  TX +------+   1k     +---------+ UPDI               |
 |                     |      +----------+    |    |                    |
 |                     |                      |    |                    |
 |                  RX +----------------------+    |                    |
 |                     |                           |                    |
 |                     +--+                     +--+                    |
 +---------------------+  |                     |  +--------------------+
                         +-+                   +-+
                         GND                   GND
```

# Features
  - Read/write flash memory
  - Read/write fuses
  - Chip erase
  - Chip reset
  - Read/write up to 1Gbps
  - Autodetection using device signature
  - CRC checking

## CRC Check
You can enable CRC checking with the ``-x`` option.  With this option enabled
during a FLASH write updiprog will do the following:

  * Read the HEX file
  * Compute a CRC-16-CCITT
  * Read the last 2-bytes of FLASH
  * If the last 2-bytes match the CRC, stop
  * Otherwise program the FLASH
  * Write the CRC to the last 2-bytes of FLASH

The end result is that with the ``-x`` option updiprog will not reprogram the
chip if the CRC indicates that the programmed firmware is the same.

The programmed CRC can also be used with the AVR's CRCSCAN feature.  With
CRCSCAN enabled, the chip will not run if the CRC at the end of FLASH does not
match the contents of FLASH memory.

# Options
```
updiprog [OPTIONS]

OPTIONS:
  -b BAUDRATE    - Serial port speed (default=115200)
  -d DEVICE      - Target device (optional)
  -c PORT        - Serial port (e.g. /dev/ttyUSB0)
  -e             - Chip erase
  -R             - Chip reset
  -F             - Read all fuses
  -f FUSE VALUE  - Write fuse
  -r FILE.HEX    - Hex file to read FLASH into
  -w FILE.HEX    - Hex file to write to FLASH
  -x             - Enable CRC checks
  -v             - Verbose logging
  -h             - Show this help screen and exit
```

# Supported devices
```
  mega1608  mega1609  mega3208  mega3209  mega4808
  mega4809  mega808   mega809   tiny1604  tiny1606
  tiny1607  tiny1614  tiny1616  tiny1617  tiny1624
  tiny1626  tiny1627  tiny202   tiny204   tiny212
  tiny214   tiny3216  tiny3217  tiny402   tiny404
  tiny406   tiny412   tiny414   tiny416   tiny417
  tiny424   tiny426   tiny427   tiny804   tiny806
  tiny807   tiny814   tiny816   tiny817   tiny824
  tiny826   tiny827

```

# Examples
## Chip Erase
    updiprog -c /dev/ttyUSB0 -e

## Program FLASH from HEX file
    updiprog -c /dev/ttyUSB0 -w firmware.hex

## Program FLASH from HEX file at 1G BAUD
    updiprog -c /dev/ttyUSB0 -w firmware.hex -b 1000000

## Read FLASH to HEX file
    updiprog -c /dev/ttyUSB0 -r tiny_fw.hex

## Read all fuses:
    updiprog -c /dev/ttyUSB0 -F

## Write 0x04 to fuse number 1 and 0x1b to fuse number 5
    updiprog -c /dev/ttyUSB0 -f 1 0x04 -f 5 0x1b

## Program FLASH from HEX file at 1G BAUD with CRC check
    updiprog -c /dev/ttyUSB0 -w firmware.hex -b 1000000 -x

## Erase the chip then program FLASH from HEX file at 1G BAUD
    updiprog -c /dev/ttyUSB0 -e -w firmware.hex -b 1000000

## Chip Reset
    updiprog -c /dev/ttyUSB0 -R
