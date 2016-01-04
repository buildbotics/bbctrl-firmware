# Makefile for the project TinyG firmware
PROJECT = tinyg
MCU     = atxmega192a3u
CLOCK   = 32000000

TARGET  = $(PROJECT).elf

# Compile falgs
CC = avr-gcc
CPP = avr-g++

COMMON = -mmcu=$(MCU)

CFLAGS += $(COMMON)
CFLAGS += -gdwarf-2 -std=gnu99 -Wall -Werror -DF_CPU=$(CLOCK)UL -Os
CFLAGS += -funsigned-bitfields -fpack-struct -fshort-enums -funsigned-char
CFLAGS += -MD -MP -MT $@ -MF build/dep/$(@F).d

# Linker flags
LDFLAGS += $(COMMON) -Wl,-u,vfprintf -lprintf_flt -lm -Wl,-Map=$(PROJECT).map
LIBS += -lm

# EEPROM flags
EEFLAGS += -j .eeprom
EEFLAGS += --set-section-flags=.eeprom="alloc,load"
EEFLAGS += --change-section-lma .eeprom=0 --no-change-warnings

# Programming flags
PROGRAMMER = avrispmkII
PDEV = usb 
AVRDUDE_OPTS = -c $(PROGRAMMER) -p $(MCU) -P $(PDEV)

FUSE0=0xff
FUSE1=0x00
FUSE2=0xfe
FUSE4=0xfe
FUSE5=0xeb

# SRC
SRC = $(wildcard src/*.c)
SRC += $(wildcard src/xmega/*.c)
OBJ = $(patsubst src/%.c,build/%.o,$(SRC))

# Build
all: $(TARGET) $(PROJECT).hex $(PROJECT).eep $(PROJECT).lss size

# Compile
build/%.o: src/%.c
	@mkdir -p $(shell dirname $@)
	$(CC) $(INCLUDES) $(CFLAGS) -c -o $@ $<

# Link
$(TARGET): $(OBJ)
	$(CC) $(LDFLAGS) $(OBJ) $(LIBS) -o $(TARGET)

%.hex: $(TARGET)
	avr-objcopy -O ihex -R .eeprom -R .fuse -R .lock -R .signature $< $@

%.eep: $(TARGET)
	avr-objcopy $(EEFLAGS) -O ihex $< $@

%.lss: $(TARGET)
	avr-objdump -h -S $< > $@

size: $(TARGET)
	avr-size -C --mcu=$(MCU) $(TARGET)

# Program
reset:
	avrdude $(AVRDUDE_OPTS)

erase:
	avrdude $(AVRDUDE_OPTS) -e

program: $(PROJECT).hex
	avrdude $(AVRDUDE_OPTS) -U flash:w:$(PROJECT).hex:i

fuses:
	avrdude $(AVRDUDE_OPTS) -U fuse0:w:$(FUSE0):m -U fuse1:w:$(FUSE1):m \
	  -U fuse2:w:$(FUSE2):m -U fuse4:w:$(FUSE4):m -U fuse5:w:$(FUSE5):m

read_fuses:
	avrdude $(AVRDUDE_OPTS) -U fuse0:r:fuse0.hex:h -U fuse1:r:fuse1.hex:h \
	  -U fuse2:r:fuse2.hex:h -U fuse4:r:fuse4.hex:h -U fuse5:r:fuse5.hex:h
	@ cat fuse?.hex

# Clean
tidy:
	rm -f $(shell find -name *~ -o -name \#*)

clean: tidy
	rm -rf $(PROJECT).elf $(PROJECT).hex $(PROJECT).eep $(PROJECT).lss \
	  $(PROJECT).map build fuse?.hex

.PHONY: tidy clean size all reset erase program fuses read_fuses

# Dependencies
-include $(shell mkdir -p build/dep) $(wildcard build/dep/*)

