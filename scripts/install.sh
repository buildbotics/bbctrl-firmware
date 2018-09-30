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

# Enable USB audio
if [ ! -e /etc/asound.conf ]; then
    (
        echo "pcm.!default {"
        echo "  type asym"
        echo "  playback.pcm \"plug:hw:0\""
        echo "  capture.pcm \"plug:dsnoop:1\""
        echo "}"
    ) > etc/asound.conf
fi

# Decrease boot delay
sed -i 's/^TimeoutStartSec=.*$/TimeoutStartSec=1/' \
    /etc/systemd/system/network-online.target.wants/networking.service

# Change to US keyboard layout
sed -i 's/^XKBLAYOUT="gb"$/XKBLAYOUT="us" # Comment stops change on upgrade/' \
    /etc/default/keyboard

# Setup USB stick automount
if [ ! -e /etc/udev/rules.d/11-automount.rules ]; then
    (
        echo 'KERNEL!="sd[a-z]*", GOTO="automount_end"'
        echo 'IMPORT{program}="/sbin/blkid -o udev -p %N"'
        echo 'ENV{ID_FS_TYPE}=="", GOTO="automount_end"'
        echo 'ENV{ID_FS_LABEL}!="", ENV{dir_name}="%E{ID_FS_LABEL}"'
        echo 'ENV{ID_FS_LABEL}=="", ENV{dir_name}="usb-%k"'
        echo 'ACTION=="add", ENV{mount_options}="relatime"'
        echo 'ACTION=="add", ENV{ID_FS_TYPE}=="vfat|ntfs", ENV{mount_options}="$env{mount_options},utf8,gid=100,umask=002,sync"'
        echo 'ACTION=="add", RUN+="/bin/mkdir -p /media/%E{dir_name}", RUN+="/bin/mount -o $env{mount_options} /dev/%k /media/%E{dir_name}"'
        echo 'ACTION=="remove", ENV{dir_name}!="", RUN+="/bin/umount -l /media/%E{dir_name}", RUN+="/bin/rmdir /media/%E{dir_name}"'
        echo 'LABEL="automount_end"'
    ) > /etc/udev/rules.d/11-automount.rules

    grep "/etc/init.d/udev restart" /etc/rc.local >/dev/null
    if [ $? -ne 0 ]; then
        echo "/etc/init.d/udev restart" >> /etc/rc.local
    fi

    sed -i 's/^\(MountFlags=slave\)/#\1/' /lib/systemd/system/systemd-udevd.service
    REBOOT=true
fi

# Install default GCode
if [ -z "$(ls -A /var/lib/bbctrl/upload)" ]; then
    cp scripts/buildbotics.gc /var/lib/bbctrl/upload/
fi

if $UPDATE_PY; then
    rm -rf /usr/local/lib/python*/dist-packages/bbctrl-*
    ./setup.py install --force
    service bbctrl start
fi

sync

if $REBOOT; then
    reboot
fi
