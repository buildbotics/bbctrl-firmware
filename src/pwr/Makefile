# Makefile for the project Bulidbotics firmware
PROJECT = bbctrl-pwr-firmware
MCU     = attiny1634
CLOCK   = 8000000
VERSION = 0.0.1

# Compile flags
CC = avr-gcc
CPP = avr-g++
COMMON = -mmcu=$(MCU)
CFLAGS += $(COMMON)
CFLAGS += -Wall -Werror
CFLAGS += -std=gnu99 -DF_CPU=$(CLOCK)UL -O3
CFLAGS += -funsigned-bitfields -fpack-struct -fshort-enums -funsigned-char
CFLAGS += -MD -MP -MT $@ -MF build/dep/$(@F).d
CFLAGS += -I. -DVERSION=\"$(VERSION)\"

# Linker flags
LDFLAGS += $(COMMON) -Wl,-u,vfprintf -lprintf_flt -lm
LIBS += -lm

# Programming flags
ifndef (PROGRAMMER)
PROGRAMMER = avrispmkII
#PROGRAMMER = jtag3pdi
endif
PDEV = usb
AVRDUDE_OPTS = -c $(PROGRAMMER) -p $(MCU) -P $(PDEV)

FUSE_EX=0xff
FUSE_HI=0xdf
FUSE_LO=0x62

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
	$(CC) $(LDFLAGS) $(OBJ) $(LIBS) -o $@

%.hex: %.elf
	avr-objcopy -O ihex -R .fuse -R .lock -R .signature $< $@

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
	avrdude $(AVRDUDE_OPTS)

erase:
	avrdude $(AVRDUDE_OPTS) -e

program: $(PROJECT).hex
	avrdude $(AVRDUDE_OPTS) -U flash:w:$(PROJECT).hex:i

verify: $(PROJECT).hex
	avrdude $(AVRDUDE_OPTS) -U flash:v:$(PROJECT).hex:i

fuses:
	avrdude $(AVRDUDE_OPTS) -U efuse:w:$(FUSE_EX):m -U lfuse:w:$(FUSE_LO):m \
	  -U hfuse:w:$(FUSE_HI):m

read_fuses:
	avrdude $(AVRDUDE_OPTS) -q -q -U efuse:r:-:h -U lfuse:r:-:h -U hfuse:r:-:h

signature:
	avrdude $(AVRDUDE_OPTS) -q -q -U signature:r:-:h

prodsig:
	avrdude $(AVRDUDE_OPTS) -q -q -U prodsig:r:-:h

usersig:
	avrdude $(AVRDUDE_OPTS) -q -q -U usersig:r:-:h

info:
	avrdude $(AVRDUDE_OPTS) -v

# Clean
tidy:
	rm -f $(shell find -name \*~ -o -name \#\*)

clean: tidy
	rm -rf $(PROJECT).elf $(PROJECT).hex $(PROJECT).lss $(PROJECT).map build

.PHONY: tidy clean size all reset erase program fuses read_fuses prodsig
.PHONY: signature usersig

# Dependencies
-include $(shell mkdir -p build/dep) $(wildcard build/dep/*)
