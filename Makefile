# Makefile for the project Bulidbotics firmware
PROJECT = buildbotics
MCU     = atxmega192a3u
CLOCK   = 32000000

TARGET  = $(PROJECT).elf

# Compile flags
CC = avr-gcc
CPP = avr-g++

COMMON = -mmcu=$(MCU)

CFLAGS += $(COMMON)
CFLAGS += -Wall -Werror # -Wno-error=unused-function
CFLAGS += -Wno-error=strict-aliasing # for _invsqrt
CFLAGS += -gdwarf-2 -std=gnu99 -DF_CPU=$(CLOCK)UL -O3 -funroll-loops
CFLAGS += -funsigned-bitfields -fpack-struct -fshort-enums -funsigned-char
CFLAGS += -MD -MP -MT $@ -MF build/dep/$(@F).d
CFLAGS += -Isrc

# Linker flags
LDFLAGS += $(COMMON) -Wl,-u,vfprintf -lprintf_flt -lm -Wl,-Map=$(PROJECT).map
LIBS += -lm

# EEPROM flags
EEFLAGS += -j .eeprom
EEFLAGS += --set-section-flags=.eeprom="alloc,load"
EEFLAGS += --change-section-lma .eeprom=0 --no-change-warnings

# Programming flags
#PROGRAMMER = avrispmkII
PROGRAMMER = jtag3pdi
PDEV = usb 
AVRDUDE_OPTS = -c $(PROGRAMMER) -p $(MCU) -P $(PDEV)

FUSE0=0xff
FUSE1=0x00
FUSE2=0xfe
FUSE4=0xff
FUSE5=0xeb

# SRC
SRC = $(wildcard src/*.c)
SRC += $(wildcard src/plan/*.c)
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
	@echo '********************************************************************'
	@avr-size -A --mcu=$(MCU) $(TARGET)
	@echo '********************************************************************'
	@avr-size -B --mcu=$(MCU) $(TARGET)
	@echo '********************************************************************'
	@avr-size -C --mcu=$(MCU) $(TARGET)

# Program
reset:
	avrdude $(AVRDUDE_OPTS)

erase:
	avrdude $(AVRDUDE_OPTS) -e

program: $(PROJECT).hex
	avrdude $(AVRDUDE_OPTS) -U flash:w:$(PROJECT).hex:i

verify: $(PROJECT).hex
	avrdude $(AVRDUDE_OPTS) -U flash:v:$(PROJECT).hex:i

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
	  $(PROJECT).map build

.PHONY: tidy clean size all reset erase program fuses read_fuses prodsig
.PHONY: signature usersig

# Dependencies
-include $(shell mkdir -p build/dep) $(wildcard build/dep/*)
