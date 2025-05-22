# Makefile for the project Bulidbotics firmware
PROJECT = bbctrl-pwr-firmware
CHIP    = tiny1616
CLOCK   = 20E6

# Compile flags
MCU    = at$(CHIP)
CC     = avr-gcc
COMMON = -mmcu=$(MCU)
CFLAGS += $(COMMON)
CFLAGS += -Wall -Werror -fshort-enums
CFLAGS += -std=gnu99 -DF_CPU=$(CLOCK) -O3
CFLAGS += -MD -MP -MT $@ -MF build/dep/$(@F).d
CFLAGS += -I. -Iinclude

# Linker flags
LDFLAGS += $(COMMON) -B$(MCU)

PROGOPTS=-c /dev/ttyUSB0 -d $(CHIP)

FUSE0=0x00 # Watch dog
FUSE1=0xed # Brown out 4.2v, enabled, sleep enabled
FUSE2=0x02 # Oscillator config
FUSE4=0x00 # Timer counter config
FUSE5=0xd4 # No CRC, NVM write block disabled, UPDI reset, no EEPROM save
FUSE6=0x00 # Startup time
FUSE7=0x00 # Application end
FUSE8=0x00 # Boot end

FUSEOPTS  = -f 0 $(FUSE0) -f 1 $(FUSE1) -f 2 $(FUSE2) -f 4 $(FUSE4)
FUSEOPTS += -f 5 $(FUSE5) -f 6 $(FUSE6) -f 7 $(FUSE7) -f 8 $(FUSE8)

# SRC
SRC = $(wildcard *.c)
OBJ = $(patsubst %.c,build/%.o,$(SRC))

# Build
all: $(PROJECT).hex size

# Compile
build/%.o: %.c
	@mkdir -p $(shell dirname $@)
	$(CC) $(INCLUDES) $(CFLAGS) -c -o $@ $<

build/%.o: %.S
	@mkdir -p $(shell dirname $@)
	$(CC) $(INCLUDES) $(CFLAGS) -c -o $@ $<

# Link
$(PROJECT).elf: $(OBJ)
	$(CC) $(LDFLAGS) $(OBJ) -o $@

%.hex: %.elf
	avr-objcopy -O ihex -R .eeprom $< $@

%.lss: %.elf
	avr-objdump -h -S $< > $@

_size:
	@echo '****************************************************************'
	@avr-size -B --mcu=$(MCU) $(SIZE_TARGET) 

size: $(PROJECT).elf
	@$(MAKE) SIZE_TARGET=$< _size

# Program
init:
	$(MAKE) erase
	-$(MAKE) fuses
	$(MAKE) program

reset:
	updiprog $(PROGOPTS) -fr

erase:
	updiprog $(PROGOPTS) -e

program: $(PROJECT).hex
	updiprog $(PROGOPTS) -e -w $(PROJECT).hex

fuses:
	updiprog $(PROGOPTS) $(FUSEOPTS)

read_fuses:
	updiprog $(PROGOPTS) -fr

# Clean
tidy:
	rm -f $(shell find -name \*~ -o -name \#\*)

clean: tidy
	rm -rf $(PROJECT).elf $(PROJECT).hex $(PROJECT).lss $(PROJECT).map build

.PHONY: tidy clean size all reset erase program fuses read_fuses

# Dependencies
-include $(shell mkdir -p build/dep) $(wildcard build/dep/*)
