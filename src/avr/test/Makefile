TARGET=firmware-test

FIRMWARE_TEST_SRC = status.c util.c axis.c report.c type.c exec.c base64.c \
  command.c commands.c vars.c state.c line.c scurve.c seek.c
FIRMWARE_TEST_SRC := $(patsubst %,../src/%,$(FIRMWARE_TEST_SRC))
FIRMWARE_TEST_SRC += $(wildcard ../src/plan/*.c) firmware-test.c hal.c

CFLAGS = -I../src -Wall -Werror -DDEBUG -g -std=gnu99
CFLAGS += -MD -MP -MT $@ -MF .dep/$(@F).d
CFLAGS += -DF_CPU=320000000
LDFLAGS = -lm

all: $(TARGET)

$(TARGET): $(FIRMWARE_TEST_SRC)
	gcc -o $@ $(FIRMWARE_TEST_SRC) $(CFLAGS) $(LDFLAGS)

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
