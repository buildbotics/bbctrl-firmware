CROSS:=arm-linux-gnueabihf-
DIR:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
obj-m:=bbserial.o
ccflags-y:=-std=gnu99 -Wno-declaration-after-statement

KVER=1.20171029-1
KPKG=raspberrypi-kernel_$(KVER).tar.gz
KURL=https://github.com/raspberrypi/linux/archive/$(KPKG)
KDIR=linux-raspberrypi-kernel_$(KVER)
export KERNEL=kernel7

KOPTS=ARCH=arm CROSS_COMPILE=$(CROSS) -C $(KDIR)

all: $(KDIR)
	$(MAKE) $(KOPTS) M=$(DIR) modules

$(KDIR): $(KPKG)
	tar xf $(KPKG)
	patch -p1 < yylloc.patch
	patch -p1 < arm-arch.patch
	$(MAKE) $(KOPTS) bcm2709_defconfig
	$(MAKE) $(KOPTS) modules_prepare

$(KPKG):
	wget $(KURL)

clean:
	$(MAKE) $(KOPTS) M=$(DIR) clean
	rm -rf $(KDIR) $(KPKG)
