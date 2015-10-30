# Buildbotics Machine Controller Setup

These instructions are for a Debian *testing* development system targeting the RaspberryPi.

## Download & install the base RPi system

```
wget https://downloads.raspberrypi.org/raspbian_latest -O raspbian-jessie.zip
unzip raspbian-jessie.zip
```

Now copy the base system to an SD card.  You need to make sure you have the correct device in the command below or you can destroy your Linux system by overwriting the disk.

```
sudo dd bs=4M if=2015-09-24-raspbian-jessie.img of=/dev/sdd
sudo sync
```

Insert the SD card into your RPi and power it on.  Plug in the network connection and determine IP address of the RPi.

## Login to the RPi

[Determine the IP address of your RPi](https://www.raspberrypi.org/documentation/troubleshooting/hardware/networking/ip-address.md).

Login:

```
ssh pi@<ip>
```

Substitute ``<ip>`` with the correct IP address.  The default password is ``raspberry``.  You should see a prompt like this: ``pi@raspberrypi ~ $``

## Configure the RPi
Update the package system, install the network auto discovery deamon, change the hostname and reboot.

```
sudo apt-get update
sudo apt-get install -y avahi-daemon
sudo sed -i 's/raspberrypi/bbctrl/' /etc/hosts /etc/hostname
sudo reboot
```

In the future you can now log in to the system like this:

```
ssh pi@bbctrl.local
```

## Install the toolchain
On the development system (i.e. not on the RPi):

```
sudo apt-get update
sudo apt-get install -y gcc-arm-linux-gnueabihf
```

# Compile and run a test program

On the developmnet system, create a file ``hello.c`` with these contents:

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
ssh pi@bbctrl.local
./hello
```

You should see the output ``Hello World!``.
