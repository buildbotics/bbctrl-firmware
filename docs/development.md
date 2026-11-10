# Buildbotics CNC Controller Development Guide

This document describes how to setup your environment for Buildbotics CNC
controller development on Debian Linux.  Development on systems other than
Debian Linux are not supported.

## Installing the Development Prerequisites

On a Debian Linux (12) system install the required packages:

    sudo apt-get update
    sudo apt-get install -y build-essential git wget binfmt-support \
      qemu-user-static parted gcc-avr avr-libc avrdude pylint python3 \
      python3-tornado curl unzip python3-setuptools nodejs coreutils bison \
      flex gcc-aarch64-linux-gnu gcc-arm-linux-gnueabihf mtools debootstrap

## Getting the Source Code

    git clone https://github.com/alexpinson5/RONtrol-firmware

## Build the Firmware

    cd RONtrol-firmware
    make

## Running the Tests

The Python test suite lives in ``tests/`` and covers the AVR command
protocol (``Cmd.py``), serial framing (``Comm.py``), config load/save
(``Config.py``), the homing/squaring logic (``State.py``) and main board
revision handling (``Board.py``).  One-time
setup:

    pip3 install -r requirements-dev.txt
    pip3 install -e .

Then run:

    make test

or ``pytest`` directly.  Coverage is reported per module; a few tests are
marked ``xfail`` to document known bugs — they start passing (reported as
``XPASS``) when the bug is fixed, at which point the marker should be
removed.

The tests fake the serial port, filesystem and controller at their seams; no
hardware is required.  They run on any OS, though one test of versioned
config paths is skipped on Windows.

## Build CAMotics Module

CAMotics is a Python module written in C++.  It must be compiled for ARM so that
it can be used on the Raspberry Pi.  This is accomplished using a chroot, qemu
and binfmt to create an emulated ARM build environment.  This is faster and
more convenient than building on the RPi itself.  All of this is automated.

    make camotics

The first time this is run it will take quite awhile as it setups up the build
environment.  You can run the above command again later to build the latest
version.

## Build the Firmware Package

    make pkg

The resulting package will be a ``.tar.bz2`` file in ``dist``.

## Upload the Firmware Package to a Buildbotics CNC Controller
If you have a controller running 2.x firmware at ``bbctrl.local``, the default
address, you can upgrade it with the new package like this:

    make update HOST=bbctrl.local PASSWORD=<pass>

Where ``<pass>`` is the controller's admin password.

## Publishing a Release

Controllers check this repository's GitHub Releases for new firmware: the web
UI compares the latest release tag with the installed version, and the
Upgrade button runs ``scripts/upgrade-bbctrl``, which downloads the release
package, checks its SHA-256 sum and installs it.  To publish:

    make release
    gh release create v<version> dist/bbctrl-<version>.tar.bz2*

``<version>`` is the ``version`` in ``package.json``.  The tag must be that
version, optionally prefixed with ``v``, and the repository must be public.
The repository is named in ``scripts/upgrade-bbctrl`` and ``src/js/app.js``.

## Supported Hardware

One package and one SD card image serve every supported controller:

| Main board | Main AVR      | Power chip | Pi     | Notes                     |
|------------|---------------|------------|--------|---------------------------|
| Rev <= 13  | ATxmega192A3U | ATtiny1634 | 3 or 4 | AVR via serial bootloader |
| Rev 14-15  | ATxmega192A3U | ATtiny3216 | 3 or 4 | Select revision in Admin  |
| Rev 16     | ATxmega192A3U | ATtiny3216 | 3 or 4 |                           |
| Rev 17+    | ATxmega256A3  | ATtiny3216 | 3 or 4 |                           |

``src/avr`` builds an image for each AVR, and ``scripts/update-bb-firmware``
picks one on every start from the chip ID read over PDI.  Rev 13 and earlier
have no PDI link to the Pi, so they are programmed through the AVR's serial
bootloader.  The power chip is only programmed from a Pi 4 (UART3), and never
on Rev 13 and earlier.  On Rev 15 and earlier the motor driver enable line is
driven by the AVR rather than the power chip; see ``src/py/bbctrl/Board.py``.

## Updating the Pwr Firmware

On main board Rev 14 and later with a Pi 4, the power chip is programmed
automatically over UPDI (the Pi's UART3) whenever bbctrl starts.  With a Pi 3,
which has no UART3, program it manually with a UPDI serial adapter on
``/dev/ttyUSB0`` attached to the power chip:

    make -C src/pwr program

``src/pwr`` is firmware for the ATtiny power chip used since Rev 14.  The
ATtiny1634 on Rev 13 and earlier keeps its factory firmware, which bbctrl
still reads.

## Initializing the main AVR firmware

The main AVR must be programmed manually the first time, or after it has been
erased.  After that the RPi programs it on every bbctrl start.  Connect an ISP
programmer to the main AVR's PDI port on the main board.

On Rev 17 and later (ATxmega256A3):

    make -C src/avr init

This sets the fuses, installs the bootloader from ``src/boot`` and programs
the firmware.  ``src/boot`` is configured for the ATxmega256A3U; check that
matches the chip before relying on it.

On Rev 16 and earlier (ATxmega192A3U), program the 192A3U build instead:

    make -C src/avr MCU=atxmega192a3u PROJECT=bbctrl-avr-firmware-192a3u \
      BUILD=build/atxmega192a3u fuses program

The bootloader in ``src/boot`` is linked for the ATxmega256A3U only, so this
tree cannot reinstall the bootloader on a 192A3U.  Boards that shipped with
1.x firmware have one.  Rev 13 and earlier depend on it, because they have no
PDI link to the Pi; do not erase it.

## Installing the RaspberryPi base system

The base system is a 64-bit Debian 12 (Bookworm) SD card image that boots on
both the Pi 3 and the Pi 4.  Build the firmware package first, then the image
(this needs root and takes a long time; the first run also builds the kernel):

    make pkg
    make image

The image is ``<date>-debian-bookworm-bbctrl.img``.  You need an SD card with
at least 8GiB.  All data on the card will be lost, so back it up if there's
anything important on it.

In the command below, make sure you have the correct device or you can
**destroy your Linux system** by overwriting the disk.  One way to do this is
to run ``sudo tail -f /var/log/syslog`` before inserting the SD card.  After
inserting the card look for log messages containing ``/dev/sdx`` where ``x`` is
a letter.  This should be the device name of the SD card.  Hit ``CTRL-C`` to
stop following the system log.

    ./scripts/write-image <date>-debian-bookworm-bbctrl.img /dev/sdx

Insert the SD card into your RPi and power it on.  On first boot it grows the
root partition to fill the card and reboots twice.  Plug in the network
connection, wired or wireless.

## Moving a 1.x Controller

Controllers that shipped with 1.x firmware run 32-bit Raspbian and cannot be
upgraded in place.  Uploading a 2.x package from the 1.x web UI is refused by
``scripts/install.sh``.  Instead:

1. Save a configuration backup from the 1.x **Admin** page.
2. Set the old SD card aside unchanged.  Putting it back restores 1.x.
3. Write a new card as described above and boot from it.
4. Restore the configuration backup from the **Admin** page.  Older
   settings are upgraded on load; review them before running a job.
5. Check **Admin > Main Board**.  Rev 13 and earlier are detected; Rev 14 and
   15 must be selected there or the motor drivers stay disabled.
6. With the motors unloaded, confirm they energize and that E-stop works
   before running a job.

On first start the AVR is reprogrammed with 2.x firmware.  Putting the old
card back afterwards runs 1.x software against 2.x AVR firmware.  Upload a 1.x
firmware package from the 1.x **Admin** page, which reprograms the AVR, before
using the machine that way.

## Logging into the Buildbotics Controller

You can ssh in to the Buildbotics Controller like so:

    ssh bbmc@bbctrl.local

The default password is ``buildbotics``.  It's best if you change this.
