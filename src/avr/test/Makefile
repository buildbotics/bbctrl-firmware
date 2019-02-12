TARGET = bbemu

SRC:=$(wildcard ../src/*.c) $(wildcard ../src/*.cpp)
OBJ:=$(patsubst %.cpp,%.o,$(patsubst %.c,%.o,$(SRC)))
OBJ:=$(patsubst ../src/%,build/%,$(OBJ))
SRC+=src/emu.c
OBJ+=build/emu.o

CFLAGS = -I../src -Isrc -Wall -Werror -DDEBUG -g -std=gnu++98
CFLAGS += -MD -MP -MT $@ -MF build/$(@F).d
CFLAGS += -DF_CPU=32000000 -Wno-class-memaccess -pthread
LDFLAGS = -lm -pthread

all: $(TARGET)

$(TARGET): $(OBJ)
	g++ -o $@ $(OBJ) $(LDFLAGS)

build/%.o: ../src/%.c
	g++ -c -o $@ $(CFLAGS) $<

build/%.o: src/%.c
	g++ -c -o $@ $(CFLAGS) $<

build/%.o: ../src/%.cpp
	g++ -c -o $@ $(CFLAGS) $<

# Clean
tidy:
	rm -f $(shell find -name \*~ -o -name \#\*)

clean: tidy
	rm -rf $(TARGET) build

.PHONY: tidy clean all

# Dependencies
-include $(shell mkdir -p build) $(wildcard build/*.d)
