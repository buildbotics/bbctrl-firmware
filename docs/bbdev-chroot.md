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
