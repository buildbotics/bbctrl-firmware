#!/bin/bash

# Check that we are on a Pi 4
PI4=false
if [ -e /proc/cpuinfo ]; then
  if ! grep -q 'Model.* Pi 4' /proc/cpuinfo; then
    echo "This firmware only works on Raspberry Pi 4"
    exit 1
  else
    PI4=true
  fi
fi

# Programs
install -C -m 0555 bin/updiprog bin/rpipdi bin/bbkbd /usr/local/bin/

# Firmwares
install -D src/pwr/bbctrl-pwr-firmware.hex src/avr/bbctrl-avr-firmware.hex \
        -t /var/lib/bbctrl/firmware/

# Stop service
if $PI4; then
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
if $PI4; then
  service bbctrl start
fi

echo "Install complete"
