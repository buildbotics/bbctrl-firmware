#!/bin/bash

ID=2

# Update the system
apt-get update && apt-get dist-upgrade -y || exit 1

# Resize FS
# TODO no /dev/root in Jessie
ROOT_PART=$(readlink /dev/root)
PART_NUM=${ROOT_PART#mmcblk0p}

if [ "$PART_NUM" != "$ROOT_PART" ]; then
    # Get the starting offset of the root partition
    PART_START=$(
        parted /dev/mmcblk0 -ms unit s p | grep "^${PART_NUM}" | cut -f 2 -d:)
    [ "$PART_START" ] &&

    fdisk /dev/mmcblk0 <<EOF &&
p
d
$PART_NUM
n
p
$PART_NUM
$PART_START
p
w
EOF

  # Now set up an init.d script to do the resize
    cat <<\EOF >/etc/init.d/resize2fs_once &&
#!/bin/sh
### BEGIN INIT INFO
# Provides:          resize2fs_once
# Required-Start:
# Required-Stop:
# Default-Start: 2 3 4 5 S
# Default-Stop:
# Short-Description: Resize the root filesystem to fill partition
# Description:
### END INIT INFO
. /lib/lsb/init-functions
case "$1" in
  start)
    log_daemon_msg "Starting resize2fs_once" &&
    resize2fs /dev/root &&
    rm /etc/init.d/resize2fs_once &&
    update-rc.d resize2fs_once remove &&
    log_end_msg $?
    ;;
  *)
    echo "Usage: $0 start" >&2
    exit 3
    ;;
esac
EOF

    chmod +x /etc/init.d/resize2fs_once &&
    update-rc.d resize2fs_once defaults
fi

# Install packages
apt-get install -y avahi-daemon avrdude minicom python3-pip i2c-tools
pip-3.2 install --upgrade tornado sockjs-tornado pyserial smbus

# Clean
apt-get autoclean

# Change hostname
sed -i "s/raspberrypi/bbctrl$ID/" /etc/hosts /etc/hostname

# Create user
useradd -m -p $(openssl passwd -1 buildbotics) -s /bin/bash bbmc
echo "bbmc ALL=(ALL) NOPASSWD: ALL" >> /etc/sudoers

# Disable console on serial port
#sed -i 's/^\(.*ttyAMA0.*\)$/# \1/' /etc/inittab
sed -i 's/console=ttyAMA0,115200 //' /boot/cmdline.txt

# Disable i2c HAT ID probe
echo -n " bcm2708.vc_i2c_override=1" >> /boot/cmdline.txt

# Disable extra gettys
sed -i 's/^\([23456]:.*\/sbin\/getty\)/#\1/' /etc/inittab

# Enable I2C
sed -i 's/#dtparam=i2c/dtparam=i2c/' /boot/config.txt
echo 'dtparam=i2c_vc=on' >> /boot/config.txt
echo i2c-bcm2708 >> /etc/modules
echo i2c-dev >> /etc/modules

# Install bbctrl w/ init.d script
cp bbctrl.init.d /etc/init.d/bbctrl
chmod +x /etc/init.d/bbctrl
update-rc.d bbctrl defaults

# Disable Pi 3 USART BlueTooth swap
echo -e "\ndtoverlay=pi3-disable-bt" >> /boot/config.txt
# sudo systemctl disable hciuart

# TODO setup input and serial device permissions in udev & forward 80 -> 8080

reboot
