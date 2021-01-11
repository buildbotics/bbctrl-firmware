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
    systemctl stop bbctrl

    # Update service
    mkdir -p /var/lib/bbctrl
    rm -f /etc/init.d/bbctrl
    cp scripts/bbctrl.service /etc/systemd/system/
    systemctl daemon-reload
    systemctl enable bbctrl
fi

if $UPDATE_AVR; then
    ./scripts/avr109-flash.py src/avr/bbctrl-avr-firmware.hex
fi

# Update config.txt
./scripts/edit-boot-config max_usb_current=1
./scripts/edit-boot-config config_hdmi_boost=8

# TODO Enable GPU
#./scripts/edit-boot-config dtoverlay=vc4-kms-v3d
#./scripts/edit-boot-config gpu_mem=16
#chmod ug+s /usr/lib/xorg/Xorg

# Fix camera
grep dwc_otg.fiq_fsm_mask /boot/cmdline.txt >/dev/null
if [ $? -ne 0 ]; then
    mount -o remount,rw /boot &&
    sed -i 's/\(.*\)/\1 dwc_otg.fiq_fsm_mask=0x3/' /boot/cmdline.txt
    mount -o remount,ro /boot
    REBOOT=true
fi

# Enable memory cgroups
grep cgroup_memory /boot/cmdline.txt >/dev/null
if [ $? -ne 0 ]; then
    mount -o remount,rw /boot &&
    sed -i 's/\(.*\)/\1 cgroup_memory=1/' /boot/cmdline.txt
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

# Setup USB stick automount
diff ./scripts/11-automount.rules /etc/udev/rules.d/11-automount.rules \
     >/dev/null
if [ $? -ne 0 ]; then
  cp ./scripts/11-automount.rules /etc/udev/rules.d/
  sed -i 's/^\(MountFlags=slave\)/#\1/' \
      /lib/systemd/system/systemd-udevd.service
  REBOOT=true
fi

# Increase swap
grep 'CONF_SWAPSIZE=1000' /etc/dphys-swapfile >/dev/null
if [ $? -ne 0 ]; then
    sed -i 's/^CONF_SWAPSIZE=.*$/CONF_SWAPSIZE=1000/' /etc/dphys-swapfile
    REBOOT=true
fi

# Install xinitrc
cp scripts/xinitrc ~pi/.xinitrc
chmod +x ~pi/.xinitrc
chown pi:pi ~pi/.xinitrc

# Install bbserial
MODSRC=src/bbserial/bbserial.ko
MODDST=/lib/modules/$(uname -r)/kernel/drivers/tty/serial/bbserial.ko
diff -q $MODSRC $MODDST 2>/dev/null >/dev/null
if [ $? -ne 0 ]; then
    cp $MODSRC $MODDST
    depmod
    REBOOT=true
fi

# Install rc.local
cp scripts/rc.local /etc/

# Install bbctrl
if $UPDATE_PY; then
    rm -rf /usr/local/lib/python*/dist-packages/bbctrl-*
    ./setup.py install --force
    service bbctrl restart
fi

sync

if $REBOOT; then
    echo "Rebooting"
    reboot
fi

echo "Install complete"
