# Makefile for the project Bulidbotics firmware
PROJECT  = bbctrl-avr-firmware
MCU      = atxmega192a3u
CLOCK    = 32000000

# SRC
SRC = $(wildcard src/*.c) $(wildcard src/*.cpp) $(wildcard src/vfd/*.c)
OBJ := $(patsubst src/%.c,build/%.o,$(SRC))
OBJ := $(patsubst src/%.cpp,build/%.o,$(OBJ))
OBJ := $(patsubst src/vfd/%.c,build/vfd/%.o,$(OBJ))
JSON = vars command messages
JSON := $(patsubst %,build/%.json,$(JSON))

all: $(PROJECT).hex $(JSON) size

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

.PHONY: all init program-boot
