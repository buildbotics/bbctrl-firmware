# Makefile for the project Bulidbotics firmware
PROJECT         = bbctrl-avr-firmware
MCU             = atxmega192a3u
CLOCK           = 32000000
VERSION         = 0.3.1

TARGET  = $(PROJECT).elf

# Compile flags
CC = avr-gcc
CPP = avr-g++

COMMON = -mmcu=$(MCU) -flto -fwhole-program

CFLAGS += $(COMMON)
CFLAGS += -Wall -Werror
CFLAGS += -Wno-error=strict-aliasing # for _invsqrt
CFLAGS += -std=gnu99 -DF_CPU=$(CLOCK)UL -O3
CFLAGS += -funsigned-bitfields -fpack-struct -fshort-enums -funsigned-char
CFLAGS += -MD -MP -MT $@ -MF build/dep/$(@F).d
CFLAGS += -Isrc -DVERSION=\"$(VERSION)\"

# Linker flags
LDFLAGS += $(COMMON) -Wl,-u,vfprintf -lprintf_flt -lm
LIBS += -lm

# EEPROM flags
EEFLAGS += -j .eeprom
EEFLAGS += --set-section-flags=.eeprom="alloc,load"
EEFLAGS += --change-section-lma .eeprom=0 --no-change-warnings

# Programming flags
PROGRAMMER = avrispmkII
#PROGRAMMER = jtag3pdi
PDEV = usb
AVRDUDE_OPTS = -c $(PROGRAMMER) -p $(MCU) -P $(PDEV)

FUSE0=0xff
FUSE1=0x00
FUSE2=0xbe
FUSE4=0xff
FUSE5=0xeb

# SRC
SRC = $(wildcard src/*.c)
SRC += $(wildcard src/plan/*.c)
OBJ = $(patsubst src/%.c,build/%.o,$(SRC))

# Boot SRC
BOOT_SRC = $(wildcard src/boot/*.S)
BOOT_SRC += $(wildcard src/boot/*.c)
BOOT_OBJ = $(patsubst src/%.c,build/%.o,$(BOOT_SRC))
BOOT_OBJ := $(patsubst src/%.S,build/%.o,$(BOOT_OBJ))

BOOT_LDFLAGS = $(LDFLAGS) -Wl,--section-start=.text=0x030000

# Build
all: $(PROJECT).hex build/vars.json boot size

boot: boot.hex boot-size

build/vars.json: src/vars.def

# Compile
build/%.json: src/%.json.in
	cpp -Isrc $< | sed '/^#.*$$/d' > $@

build/%.o: src/%.c
	@mkdir -p $(shell dirname $@)
	$(CC) $(INCLUDES) $(CFLAGS) -c -o $@ $<

build/%.o: src/%.S
	@mkdir -p $(shell dirname $@)
	$(CC) $(INCLUDES) $(CFLAGS) -c -o $@ $<

# Link
$(TARGET): $(OBJ)
	$(CC) $(LDFLAGS) $(OBJ) $(LIBS) -o $@

boot.elf: $(BOOT_OBJ)
	$(CC) $(BOOT_LDFLAGS) $(BOOT_OBJ) -o $@

%.hex: %.elf
	avr-objcopy -O ihex -R .eeprom -R .fuse -R .lock -R .signature $< $@

%.eep: %.elf
	avr-objcopy $(EEFLAGS) -O ihex $< $@

%.lss: %.elf
	avr-objdump -h -S $< > $@

_size:
	@for X in A B C; do\
	  echo '****************************************************************' ;\
	  avr-size -$$X --mcu=$(MCU) $(SIZE_TARGET) ;\
	done

boot-size: boot.elf
	@$(MAKE) SIZE_TARGET=$< _size

size: $(TARGET)
	@$(MAKE) SIZE_TARGET=$< _size

# Program
init:
	$(MAKE) erase
	-$(MAKE) fuses
	$(MAKE) program-boot
	$(MAKE) program

reset:
	avrdude $(AVRDUDE_OPTS)

erase:
	avrdude $(AVRDUDE_OPTS) -e

program: $(PROJECT).hex
	avrdude $(AVRDUDE_OPTS) -U flash:w:$(PROJECT).hex:i

verify: $(PROJECT).hex
	avrdude $(AVRDUDE_OPTS) -U flash:v:$(PROJECT).hex:i

program-boot: boot.hex
	avrdude $(AVRDUDE_OPTS) -U flash:w:boot.hex:i

fuses:
	avrdude $(AVRDUDE_OPTS) -U fuse0:w:$(FUSE0):m -U fuse1:w:$(FUSE1):m \
	  -U fuse2:w:$(FUSE2):m -U fuse4:w:$(FUSE4):m -U fuse5:w:$(FUSE5):m

read_fuses:
	avrdude $(AVRDUDE_OPTS) -q -q -U fuse0:r:-:h -U fuse1:r:-:h -U fuse2:r:-:h \
	  -U fuse4:r:-:h -U fuse5:r:-:h

signature:
	avrdude $(AVRDUDE_OPTS) -q -q -U signature:r:-:h

prodsig:
	avrdude $(AVRDUDE_OPTS) -q -q -U prodsig:r:-:h

usersig:
	avrdude $(AVRDUDE_OPTS) -q -q -U usersig:r:-:h

# Clean
tidy:
	rm -f $(shell find -name \*~ -o -name \#\*)

clean: tidy
	rm -rf $(PROJECT).elf $(PROJECT).hex $(PROJECT).eep $(PROJECT).lss \
	  $(PROJECT).map build boot.hex boot.elf

.PHONY: tidy clean size all reset erase program fuses read_fuses prodsig
.PHONY: signature usersig

# Dependencies
-include $(shell mkdir -p build/dep) $(wildcard build/dep/*)
