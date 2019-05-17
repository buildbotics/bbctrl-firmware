CROSS := arm-linux-gnueabihf-
DIR:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
obj-m := bbserial.o
ccflags-y := -std=gnu99 -Wno-declaration-after-statement

all:
	$(MAKE) ARCH=arm CROSS_COMPILE=$(CROSS) -C kernel M=$(DIR) modules

clean:
	$(MAKE) ARCH=arm CROSS_COMPILE=$(CROSS) -C kernel M=$(DIR) clean
