# Install the cross compiler

    sudo apt-get update
    sudo apt-get install -y gcc-arm-linux-gnueabihf

# Determine the correct kernel version on the target pi

    dpkg-query -l raspberrypi-kernel

# Get the kernel source for the correct kernel version

    wget https://github.com/raspberrypi/linux/archive/raspberrypi-kernel_1.20171029-1.tar.gz
    tar xf raspberrypi-kernel_1.20171029-1.tar.gz
    cd linux-raspberrypi-kernel_1.20171029-1

# Prep the kernel source

    KERNEL=kernel7
    make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- bcm2709_defconfig
    make -j 8 ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- modules_prepare

# Create hello.c module

    #include <linux/init.h>
    #include <linux/module.h>

    MODULE_LICENSE("GPL");

    static int hello_init(void) {
      printk(KERN_ALERT "Hello world!\n");
      return 0;
    }

    static void hello_exit(void) {
      printk(KERN_ALERT "Goodbye cruel world!\n");
    }

    module_init(hello_init);
    module_exit(hello_exit);

# Create a Makefile

    CROSS := arm-linux-gnueabihf-
    DIR:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
    obj-m := hello.o

    all:
        $(MAKE) ARCH=arm CROSS_COMPILE=$(CROSS) -C kernel M=$(DIR) modules

    clean:
        $(MAKE) ARCH=arm CROSS_COMPILE=$(CROSS) -C kernel M=$(DIR) clean

# Compile the module

    ln -sf path/to/rpi-kernel kernel
    make

# Copy module to rpi

    scp hello.ko pi.local:

# Load it on the pi

    sudo insmod hello.ko

# Look for message in syslog

    tail /var/log/syslog
