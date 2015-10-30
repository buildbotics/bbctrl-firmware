#!/bin/bash

# Update the system
apt-get update
apt-get dist-upgrade -y

# Install the auto discovery daemon
apt-get install -y avahi-daemon

# Change the hostname
sed -i 's/raspberrypi/bbctrl/' /etc/hosts /etc/hostname

# Create bb user
useradd -m -p buildbotics -s /bin/bash user

# Remove pi user
cd /
userdel -rf pi

reboot
