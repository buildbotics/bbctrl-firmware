#!/bin/bash

# Update the system
apt-get update
apt-get dist-upgrade -y

# Install the auto discovery daemon
apt-get install -y avahi-daemon

# Clean
apt-get autoclean

# Change the hostname
sed -i 's/raspberrypi/bbctrl/' /etc/hosts /etc/hostname

# Create bb user
useradd -m -p $(openssl passwd -1 buildbotics) -s /bin/bash bbmc
echo "bbmc ALL=(ALL) NOPASSWD: ALL" >> /etc/sudoers

reboot
