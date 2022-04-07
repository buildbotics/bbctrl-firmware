#!/bin/bash

UPDATE_AVR=true
UPDATE_PWR=true
UPDATE_PY=true
REBOOT=false

while [ $# -gt 0 ]; do
  case "$1" in
    --no-avr) UPDATE_AVR=false ;;
    --no-pwr) UPDATE_PWR=false ;;
    --no-py)  UPDATE_PY=false  ;;
  esac
  shift 1
done

if [ -e /etc/systemd/system/bbctrl.service ]; then
  service bbctrl stop
fi

# Find devices
if [ -e /dev/ttyAMA0 ]; then
  AVR_DEV=/dev/ttyAMA0
  AVR_RESET=27
elif [ -e /dev/ttyS2 ]; then
  AVR_DEV=/dev/ttyS2
  AVR_RESET=117
else
  >&2 echo "Cannot find AVR serial device."
  UPDATE_AVR=false
fi

if $UPDATE_PY; then
  # Update service
  mkdir -p /var/lib/bbctrl
  rm -f /etc/init.d/bbctrl
  install scripts/bbctrl.service /etc/systemd/system/
  systemctl daemon-reload
  systemctl enable bbctrl
fi

if $UPDATE_AVR; then
  ./scripts/avr109-flash -d $AVR_DEV -r $AVR_RESET \
    src/avr/bbctrl-avr-firmware.hex
fi

# Update config.txt
./scripts/edit-boot-config max_usb_current=1
./scripts/edit-boot-config config_hdmi_boost=8

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
  install ./scripts/11-automount.rules /etc/udev/rules.d/
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
install -o pi -g pi -m 0555 scripts/xinitrc ~pi/.xinitrc

# Install bbserial
MODSRC=src/bbserial/bbserial.ko
MODDST=/lib/modules/$(uname -r)/kernel/drivers/tty/serial/bbserial.ko
diff -q $MODSRC $MODDST 2>/dev/null >/dev/null
if [ $? -ne 0 ]; then
  install $MODSRC $MODDST
  depmod
  REBOOT=true
fi

# Install splash
install -D src/splash/* -t /usr/share/plymouth/themes/buildbotics/
plymouth-set-default-theme -R buildbotics

# Install rc.local
install scripts/rc.local /etc/

# Install updiprog
install -C -m 0555 share/updiprog/updiprog /usr/local/bin/

# Install rpipdi
install -C -m 0555 share/rpipdi/rpipdi /usr/local/bin/

# Install bbkbd
diff share/bbkbd/bbkbd /usr/local/bin/bbkbd 2>&1 >/dev/null
if [ $? -ne 0 ]; then
  REBOOT=true
  killall -9 bbkbd
  install -m 0555 share/bbkbd/bbkbd /usr/local/bin/
fi

# Remove xontab keyboard
rm -rf /home/pi/.config/chromium/Default/Extensions/pflmllfnnabikmfkkaddkoolinlfninn

if $UPDATE_PWR; then
  PWR_OPTS="-c /dev/ttyAMA1 -b 500000 -x"
  /usr/local/bin/updiprog $PWR_OPTS -w src/pwr2/bbctrl-pwr-firmware.hex
fi

# Install bbctrl
if $UPDATE_PY; then
  rm -rf /usr/local/lib/python*/dist-packages/bbctrl-*
  ./setup.py install --force
fi

sync

if $REBOOT; then
  echo "Rebooting"
  reboot

else
  service bbctrl start
fi

echo "Install complete"
