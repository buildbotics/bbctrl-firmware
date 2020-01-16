This document describes how to setup a Buildbotics development environment
on a Debian based system inside a chroot.  Building in the chroot ensures that
you have a clean and consistent build environment unaltered by other packages
or manual changes.

# Install packages required to create chroot

    sudo apt-get update
    sudo apt-get install binutils debootstrap

# Create chroot environment

    mkdir bbdev
    sudo debootstrap --arch amd64 stable bbdev http://deb.debian.org/debian

# Copy downloaded files (optional)
To speed things up you can copy to large downloads, if you already have them,
into the chroot.

    sudo mkdir -p bbdev/opt/bbctrl-firmware/src/bbserial/
    sudo cp 2017-11-29-raspbian-stretch-lite.zip bbdev/opt/bbctrl-firmware/
    sudo cp raspberrypi-kernel_1.20171029-1.tar.gz bbdev/opt/bbctrl-firmware/src/bbserial/

# Enter the chroot

    sudo mount --bind /proc bbdev/proc
    sudo mount --rbind /sys bbdev/sys
    sudo mount --rbind /dev bbdev/dev
    sudo chroot bbdev
    cd /opt

Now, follow the instructions in [development.md](development.md) from with in
the chroot.

# Exit the chroot
To exit the chroot:

    exit
    sudo umount bbdev/dev
    sudo umount bbdev/sys
    sudo umount bbdev/proc
