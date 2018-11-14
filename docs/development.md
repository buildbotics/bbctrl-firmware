# Buildbotics CNC Controller Development Guide

This document describes how to setup your environment for Buildbotics CNC
controller development on Debian Linux.  Development on systems other than
Debian Linux are not supported.

## Installing the Development Prerequisites

On a Debian Linux (9.6.0 stable) system install the required packages:

    sudo apt-get update
    sudo apt-get install -y build-essential git wget binfmt-support qemu \
      parted gcc-avr avr-libc avrdude pylint3 python3 python3-tornado curl \
      unzip python3-setuptools
    curl -sL https://deb.nodesource.com/setup_11.x | sudo -E bash -
    sudo apt-get install -y nodejs

## Getting the Source Code

    git clone https://github.com/buildbotics/bbctrl-firmware

## Build the Firmware

    cd bbctrl-firmware
    make

## Build GPlan Module

GPlan is a Python module written in C++.  It must be compiled for ARM so that
it can be used on the Raspberry Pi.  This is accomplished using a chroot, qemu
and binfmt to create an emulated ARM build environment.  This is faster and
more convenient than building on the RPi itself.  All of this is automated.

    make gplan

The first time this is run it will take quite awhile as it setups up the build
environment.  You can run the above command again later to build the latest
version.

## Build the Firmware Package

    make pkg

The resulting package will be a ``.tar.bz2`` file in ``dist``.

## Upload the Firmware Package to a Buildbotics CNC Controller
If you have a Buildbotics CNC controller at ``bbctrl.local``, the default
address, you can upgrade it with the new package like this:

    make update HOST=bbctrl.local PASSWORD=<pass>

Where ``<pass>`` is the controller's admin password.

## Updating the Pwr Firmware

The Pwr firmware must be uploaded manually using an ISP programmer.  With the
programmer attached to the pwr chip ISP port on the Builbotics controller's
main board run the following:

    make -C src/pwr program

## Initializing the main AVR firmware

The main AVR must also be programmed manually the first time.  Later it will be
automatically programmed by the RPi as part of the firmware install.  To perform
the initial AVR programming connec the ISP programmer to the main AVR's ISP port
on the Buildbotics controller's main board and run the following:

    make -C src/avr init

This will set the fuses, install the bootloader and program the firmware.

## Installing the RaspberryPi base system

Download the latest Buildbotics CNC controller base image and decompress it:

    wget \
      https://buildbotics.com/upload/2018-05-15-raspbian-stretch-bbctrl.img.xz
    xz -d 2018-05-15-raspbian-stretch-bbctrl.img.xz

Now copy the base system to an SD card.  You need a card with at least 8GiB.
After installing the RPi system all data on the SD card will be lost.  So make
sure you back up the SD card if there's anything important on it.

In the command below, make sure you have the correct device or you can
**destroy your Linux system** by overwriting the disk.  One way to do this is
to run ``sudo tail -f /var/log/syslog`` before inserting the SD card.  After
inserting the card look for log messages containing ``/dev/sdx`` where ``x`` is
a letter.  This should be the device name of the SD card.  Hit ``CTRL-C`` to
stop following the system log.

    sudo dd bs=4M if=2015-05-05-raspbian-wheezy.img of=/dev/sde
    sudo sync

The first command takes awhile and does not produce any output until it's done.

Insert the SD card into your RPi and power it on.  Plug in the network
connection, wired or wireless.

## Logging into the Buildbotics Controller

You can ssh in to the Buildbotics Controller like so:

    ssh bbmc@bbctrl.local

The default password is ``buildbotics``.  It's best if you change this.
