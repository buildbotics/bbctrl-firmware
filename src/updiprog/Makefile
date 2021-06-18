TARGET=updiprog

SRC := $(wildcard src/*.c)
OBJ := $(patsubst src/%.c,build/%.o,$(SRC))

CFLAGS += -O3 -Wall -Werror -Isrc
CFLAGS += -MD -MP -MT $@ -MF build/dep/$(@F).d


all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) -o $@ $(OBJ) $(LDFLAGS)

build/%.o: src/%.c
	@mkdir -p $(shell dirname $@)
	$(CC) $(CFLAGS) -c -o $@ $<

tidy:
	rm -f *~ \#*

clean: tidy
	rm -rf build $(TARGET)

.PHONY: all tidy clean

# Dependencies
-include $(shell mkdir -p build/dep) $(wildcard build/dep/*.d)
