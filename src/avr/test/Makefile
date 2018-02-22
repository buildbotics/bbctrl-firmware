TESTS=planner-test

PLANNER_TEST_SRC = status.c util.c axis.c report.c type.c exec.c base64.c \
  command.c commands.c vars.c state.c line.c scurve.c seek.c
PLANNER_TEST_SRC := $(patsubst %,../src/%,$(PLANNER_TEST_SRC))
PLANNER_TEST_SRC += $(wildcard ../src/plan/*.c) planner-test.c hal.c

CFLAGS = -I../src -Wall -Werror -DDEBUG -g -std=gnu99
CFLAGS += -MD -MP -MT $@ -MF .dep/$(@F).d
CFLAGS += -DF_CPU=320000000
LDFLAGS = -lm

all: $(TESTS)

planner-test: $(PLANNER_TEST_SRC)
	gcc -o $@ $(PLANNER_TEST_SRC) $(CFLAGS) $(LDFLAGS)

%.csv: %.gc planner-test
	./planner-test < $< | grep -E '^-?[0-9.]+,'

%-test: %.gc planner-test
	./planner-test < $<

%-plot: %.gc planner-test
	./planner-test < $< | grep -E '^-?[0-9.]+,' | ./plot.py

# Clean
tidy:
	rm -f $(shell find -name \*~ -o -name \#\*) .dep

clean: tidy
	rm -rf $(TESTS)

.PHONY: tidy clean all

# Dependencies
-include $(shell mkdir -p .dep) $(wildcard .dep/*)
