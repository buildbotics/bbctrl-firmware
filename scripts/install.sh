#!/bin/bash

# Check that we are on a Pi 3 or Pi 4.  /proc is not mounted when this runs
# inside the image build chroot.
ON_PI=false
if [ -e /proc/cpuinfo ]; then
  if ! grep -q 'Model.* Pi [34]' /proc/cpuinfo; then
    echo "This firmware only works on Raspberry Pi 3 or 4"
    exit 1
  else
    ON_PI=true
  fi
fi

# This firmware needs the 64-bit 2.x SD card image.  A 1.x controller runs
# 32-bit Raspbian, and its firmware upload stops bbctrl before running this
# script, so restart it rather than leave the controller down.
if [ "$(dpkg --print-architecture)" != arm64 ]; then
  echo "This firmware needs the 64-bit 2.x SD card image." \
       "See 'Moving a 1.x Controller' in docs/development.md."
  if $ON_PI; then systemctl start bbctrl; fi
  exit 1
fi

# Self-signed SSL Certificate
if [ ! -d /etc/bbctrl ]; then
  mkdir -m 0744 /etc/bbctrl
fi

SSLKEY=/etc/bbctrl/ssl.key
SSLCRT=/etc/bbctrl/ssl.crt
if [ ! -e $SSLKEY ]; then
  openssl req -x509 -nodes -days $((365 * 100)) -newkey rsa:2048 \
    -keyout $SSLKEY -out $SSLCRT -subj "/C=US/O=Buildbotics LLC/CN=buildbotics"
fi

# Programs
install -C -m 0555 bin/updiprog bin/rpipdi bin/bbkbd /usr/local/bin/

# Firmwares
install -D src/pwr/bbctrl-pwr-firmware.hex src/avr/bbctrl-avr-firmware.hex \
        src/avr/bbctrl-avr-firmware-192a3u.hex -t /var/lib/bbctrl/firmware/

# Stop service
if $ON_PI; then
  service bbctrl stop
fi

# bbctrl
pip3 install --break-system-packages .

# camotics.so
PKG_DIR=$(printf "import bbctrl\nprint(bbctrl.__file__)"|python3 -)
PKG_DIR=$(dirname "$(dirname "$PKG_DIR")")
install bin/camotics.so $PKG_DIR/bbctrl/

# Synchronize files
sync

# Restart service
if $ON_PI; then
  service bbctrl start
fi

echo "Install complete"
