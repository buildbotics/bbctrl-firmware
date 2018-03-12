# Makefile for the project Bulidbotics firmware
PROJECT  = bbctrl-avr-firmware
MCU      = atxmega192a3u
CLOCK    = 32000000

# SRC
SRC = $(wildcard src/*.c) $(wildcard src/*.cpp)
OBJ = $(patsubst src/%.cpp,build/%.o,$(patsubst src/%.c,build/%.o,$(SRC)))
JSON = vars command messages
JSON := $(patsubst %,build/%.json,$(JSON))

include Makefile.common

CFLAGS += -Isrc

# Build
all: $(PROJECT).hex $(JSON) size

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

.PHONY: all init program-boot
