TARGETS = firmware-test jogsim

FIRMWARE_TEST_SRC = status.c util.c axis.c report.c type.c exec.c base64.c \
  command.c commands.c vars.c state.c line.c seek.c SCurve.cpp
FIRMWARE_TEST_SRC := $(patsubst %,../src/%,$(FIRMWARE_TEST_SRC))
FIRMWARE_TEST_SRC += firmware-test.c hal.c

JOGSIM_SRC = ../src/SCurve.cpp jogsim.cpp

CFLAGS = -I../src -Wall -Werror -DDEBUG -g -std=gnu++98
CFLAGS += -MD -MP -MT $@ -MF .dep/$(@F).d
CFLAGS += -DF_CPU=320000000
LDFLAGS = -lm

all: $(TARGETS)

firmware-test: $(FIRMWARE_TEST_SRC)
	g++ -o $@ $(FIRMWARE_TEST_SRC) $(CFLAGS) $(LDFLAGS)

jogsim: $(JOGSIM_SRC)
	g++ -o $@ $(JOGSIM_SRC) $(CFLAGS) $(LDFLAGS)

%.csv: %.gc firmware-test
	./firmware-test < $< | grep -E '^-?[0-9.]+,'

%-test: %.gc firmware-test
	./firmware-test < $<

%-plot: %.gc firmware-test
	./firmware-test < $< | grep -E '^-?[0-9.]+,' | ./plot.py

# Clean
tidy:
	rm -f $(shell find -name \*~ -o -name \#\*) .dep

clean: tidy
	rm -rf $(TESTS)

.PHONY: tidy clean all

# Dependencies
-include $(shell mkdir -p .dep) $(wildcard .dep/*)
