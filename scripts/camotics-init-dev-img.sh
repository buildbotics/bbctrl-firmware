#!/bin/bash -e

export LC_ALL=C
cd /mnt/host

# Update the system
apt-get update
#apt-get dist-upgrade -y

# Install packages
apt-get install -y scons build-essential libssl-dev python3-dev nodejs-dev
