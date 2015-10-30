# Buildbotics Machine Controller Setup

These instructions are for a Debian *testing* development system targeting the RaspberryPi.

## Download & install the base RPi system

```
wget https://downloads.raspberrypi.org/raspbian/images/raspbian-2015-05-07/2015-05-05-raspbian-wheezy.zip
unzip 2015-05-05-raspbian-wheezy.zip
```

Now copy the base system to an SD card.  You need a card with at least 4GiB.  After installing the RPi system all data on the SD card will be lost.  So make sure you back up the SD card if there's anything important on it

In the command below, make sure you have the correct device or you can **destroy your Linux system** by overwriting the disk.  One way to do this is to run ``sudo tail -f /var/log/syslog`` before inserting the SD card.  After inserting the card look for log messages containing ``/dev/sdx`` where ``x`` is a letter.  This should be the device name of the SD card.  Hit ``CTRL-C`` to stop following the system log.

```
sudo dd bs=4M if=2015-05-05-raspbian-wheezy.img of=/dev/sde
sudo sync
```

The first command takes awhile and does not produce any output until it's done.

Insert the SD card into your RPi and power it on.  Plug in the network connection, wired or wireless.

## Login to the RPi

[Determine the IP address of your RPi](https://www.raspberrypi.org/documentation/troubleshooting/hardware/networking/ip-address.md).

Login:

```
ssh pi@<ip>
```

Substitute ``<ip>`` with the correct IP address.  The default password is ``raspberry``.  You should see a prompt like this: ``pi@raspberrypi ~ $``, but in color.

## Configure the RPi
Copy the ``setup.sh`` script to the RPi and run it as root:

```
scp setup.sh pi@<ip>:
ssh pi@<ip> sudo ./setup.sh
```

This will take some time and will end by rebooting the RPi.  After this script has run you can log in to the RPi with out typing the IP addrerss like this:

```
ssh bbmc@bbctrl.local
```

## Install the toolchain
On the development system (i.e. not on the RPi):

```
sudo apt-get update
sudo apt-get install -y gcc-arm-linux-gnueabihf
```

## Compile and run a test program

On the development system, create a file ``hello.c`` with these contents:

```
#include <stdio.h>

int main(int argc, char *argv[]) {
  printf("Hello World!\n");
  return 0;
}
```

Compile it like this:

```
arm-linux-gnueabihf-gcc hello.c -o hello
```

Copy ``hello`` to the RPi:

```
scp hello pi@bbctrl.local:
```

Login to the system and run ``hello`` like this:

```
ssh bbmc@bbctrl.local
./hello
```

You should see the output ``Hello World!``.
