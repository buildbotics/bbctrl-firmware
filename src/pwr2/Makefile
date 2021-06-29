# Makefile for the project Bulidbotics firmware
PROJECT = bbctrl-pwr-firmware
MCU     = attiny3216
CLOCK   = 20E6

# Compile flags
CC = avr-gcc
COMMON = -mmcu=$(MCU)
CFLAGS += $(COMMON)
CFLAGS += -Wall -Werror
CFLAGS += -std=gnu99 -DF_CPU=$(CLOCK) -O3
CFLAGS += -MD -MP -MT $@ -MF build/dep/$(@F).d
CFLAGS += -I. -Iinclude

# Linker flags
LDFLAGS += $(COMMON) -B$(MCU)

PROGOPTS=-c /dev/ttyUSB0 -d tiny321x

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
	@for X in A B C; do\
	  echo '****************************************************************' ;\
	  avr-size -$$X --mcu=$(MCU) $(SIZE_TARGET) ;\
	done

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
	false

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
