TESTS=planner-test

PLANNER_TEST_SRC = gcode_parser.c machine.c status.c util.c axis.c report.c \
  homing.c probing.c command.c vars.c
PLANNER_TEST_SRC := $(patsubst %,../src/%,$(PLANNER_TEST_SRC))
PLANNER_TEST_SRC += $(wildcard ../src/plan/*.c) planner-test.c hal.c

CFLAGS = -I../src -Wall -Werror -DDEBUG -g -std=gnu99
CFLAGS += -MD -MP -MT $@ -MF .dep/$(@F).d
CFLAGS += -DF_CPU=320000000
LDFLAGS = -lm

all: $(TESTS)

planner-test: $(PLANNER_TEST_SRC)
	gcc -o $@ $(PLANNER_TEST_SRC) $(CFLAGS) $(LDFLAGS)

test: planner-test
	./$< < test.gc

csv: planner-test
	@$(MAKE) -s test | grep -E '^-?[0-9.]+,'

plot: planner-test
	@$(MAKE) -s csv | ./plot_velocity.py

# Clean
tidy:
	rm -f $(shell find -name \*~ -o -name \#\*)

clean: tidy
	rm -rf $(TESTS)

.PHONY: tidy clean all test csv plot

# Dependencies
-include $(shell mkdir -p .dep) $(wildcard .dep/*)
