# Makefile for the project Bulidbotics firmware
PROJECT = bbctrl-avr-firmware
MCU     = atxmega256a3
CLOCK   = 32000000
BUILD  ?= build

# Main boards up to and including Rev 16 carry an ATxmega192A3U.  The same
# source is built for it as a second image; update-bb-firmware picks the image
# that matches the chip it finds.
LEGACY_MCU     = atxmega192a3u
LEGACY_PROJECT = bbctrl-avr-firmware-192a3u

# SRC
SRC = $(wildcard src/*.c) $(wildcard src/*.cpp) $(wildcard src/vfd/*.c)
OBJ := $(patsubst src/%.c,$(BUILD)/%.o,$(SRC))
OBJ := $(patsubst src/%.cpp,$(BUILD)/%.o,$(OBJ))
OBJ := $(patsubst src/vfd/%.c,$(BUILD)/vfd/%.o,$(OBJ))
JSON = vars command messages
JSON := $(patsubst %,build/%.json,$(JSON))

all: $(PROJECT).hex $(LEGACY_PROJECT).hex $(JSON) size

# Rebuilt by a sub-make with its own object directory, which tracks its own
# dependencies, so always delegate to it.  The guard stops the sub-make, which
# builds this same file name, from matching this rule again.
ifneq ($(MCU),$(LEGACY_MCU))
.PHONY: $(LEGACY_PROJECT).hex
$(LEGACY_PROJECT).hex:
	$(MAKE) MCU=$(LEGACY_MCU) PROJECT=$(LEGACY_PROJECT) \
	  BUILD=build/$(LEGACY_MCU) $@
endif

include Makefile.common

CFLAGS += -Isrc

# Build
$(PROJECT).elf: $(OBJ)
	$(CC) $(LDFLAGS) $(OBJ) $(LIBS) -o $@


# JSON
build/%.json: src/%.json.in src/%.def
	cpp -Isrc $< | sed "/^#.*$$/d;s/'\(.\)'/\"\1\"/g" > $@

# Program
init:
	$(MAKE) erase
	-$(MAKE) fuses
	$(MAKE) fuses
	$(MAKE) program-boot
	$(MAKE) program

program-boot:
	$(MAKE) -C ../boot program

clean: clean-legacy
clean-legacy:
	rm -f $(LEGACY_PROJECT).elf $(LEGACY_PROJECT).hex

.PHONY: all init program-boot clean-legacy
