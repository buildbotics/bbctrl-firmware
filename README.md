![Now on Kickstarter](https://buildbotics.com/images/buildbotics_now_on_kickstarter.png)
[http://buildbotics.com/kickstarter](http://buildbotics.com/kickstarter)

# Buildbotics Machine Controller Setup

These instructions are for a Debian *testing* development system targeting the RaspberryPi.

## Download & install the base RPi system

```
https://downloads.raspberrypi.org/raspbian/images/raspbian-2015-09-28/2015-09-24-raspbian-jessie.zip
unzip 2015-09-24-raspbian-jessie.zip
```

or

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
Copy the ``scripts/setup_rpi.sh`` script to the RPi and run it as root:

```
scp scripts/setup_rpi.sh pi@<ip>:
ssh pi@<ip> sudo ./setup_rpi.sh
```

This will take some time and will end by rebooting the RPi.  After this script has run you can log in to the RPi with out typing the IP addrerss like this:

```
ssh bbmc@bbctrl.local
```

## Install the RPi toolchain
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

## Install the AVR toolchain
Install the following tools for programming the AVR:

```
sudo apt-get install -y avrdude gcc-avr
```

## Connect to TinyG

From RPi terminal

```
minicom -b 115200 -o -D /dev/ttyAMA0
```

You should see a prompt.


## Program TinyG

```
avrdude -c avrispmkII -p ATxmega128A3U -P usb -U flash:w:tinyg.hex:i
```


## Setup Python Development


# Huanyang Spindle Setup

Connections:

 * pin 13 -> Rs+
 * pin 14 -> Rs-

Program the following settings:

 * PD013 = 8 (reset to factory settings)
 * PD005 = 400 (max frequency 400Hz)
 * PD004 = 400 (base frequency 400Hz)
 * PD003 = 400 (main frequency 400Hz)
 * PD001 = 2 (set communication port as source of run commands)
 * PD002 = 2 (set communication port as source of operating frequency)
 * PD163 = 1 (slave address 1)
 * PD164 = 1 (baud rate 9600 bps)
 * PD165 = 3 (8N1 for RTU mode)

For a 1.5KW spindle:

 * PD006 = 2.5 (intermediate frequency 2.5Hz)
 * PD008 = 220 (max voltage 220V)
 * PD009 = 15 (intermediate voltage 15V)
 * PD010 = 8 (min voltage 8V)
 * PD011 = 120 (frequency lower limit 120Hz, to limit lower RPM settings)
 * PD014 = 5.0 (acceleration time, 5 seconds)
 * PD015 = 0.8 (deceleration time; any more trips the VFD)
 * PD025 = 1 (starting mode: frequency track)
 * PD142 = 7 (max current 7 A)
 * PD143 = 2 (specific to my 1.5 KW spindle: number of poles - 2)
 * PD144 = 3000 (multiplied by PD010 = 3000 * 8 = 24,000 RPM)

See manual for settings for other spindles.
