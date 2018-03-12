# Makefile for Bulidbotics step-test
PROJECT  = step-test
MCU      = atxmega192a3u
CLOCK    = 32000000

# SRC
SRC = usart.c lcd.c pins.c hardware.c
SRC := $(wildcard *.c) $(patsubst %,../src/%,$(SRC))

include ../Makefile.common

CFLAGS += -I../src -I.

# Build
all: $(PROJECT).hex size

$(PROJECT).elf: $(SRC)
	$(CC) $(SRC) $(CFLAGS) $(LDFLAGS) $(LIBS) -o $@

.PHONY: all
