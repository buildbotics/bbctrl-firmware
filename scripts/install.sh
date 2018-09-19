#!/bin/bash

UPDATE_AVR=true
UPDATE_PY=true
REBOOT=false

while [ $# -gt 0 ]; do
    case "$1" in
        --no-avr) UPDATE_AVR=false ;;
        --no-py) UPDATE_PY=false ;;
    esac
    shift 1
done


if $UPDATE_PY; then
    if [ -e /var/run/bbctrl.pid ]; then
        service bbctrl stop
    fi
fi

if $UPDATE_AVR; then
    ./scripts/avr109-flash.py src/avr/bbctrl-avr-firmware.hex
fi

# Increase USB current
grep max_usb_current /boot/config.txt >/dev/null
if [ $? -ne 0 ]; then
    mount -o remount,rw /boot &&
    echo max_usb_current=1 >> /boot/config.txt
    mount -o remount,ro /boot
fi


# Fix camera
grep dwc_otg.fiq_fsm_mask /boot/cmdline.txt >/dev/null
if [ $? -ne 0 ]; then
    mount -o remount,rw /boot &&
    sed -i 's/\(.*\)/\1 dwc_otg.fiq_fsm_mask=0x3/' /boot/cmdline.txt
    mount -o remount,ro /boot
    REBOOT=true
fi

# Remove Hawkeye
if [ -e /etc/init.d/hawkeye ]; then
    apt-get remove --purge -y hawkeye
fi

# Decrease boot delay
sed -i 's/^TimeoutStartSec=.*$/TimeoutStartSec=1/' \
    /etc/systemd/system/network-online.target.wants/networking.service

# Change to US keyboard layout
sed -i 's/^XKBLAYOUT="gb"$/XKBLAYOUT="us" # Comment stops change on upgrade/' \
    /etc/default/keyboard

if $UPDATE_PY; then
    rm -rf /usr/local/lib/python*/dist-packages/bbctrl-*
    ./setup.py install --force
    service bbctrl start
fi

sync

if $REBOOT; then
    reboot
fi
