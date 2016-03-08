#!/bin/bash

ID=2

# Update the system
apt-get update
apt-get dist-upgrade -y

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

# Install pacakges
apt-get install -y avahi-daemon avrdude minicom python-pip
pip install tornado sockjs-tornado pyserial

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

# TODO install bbctrl w/ init.d script

reboot
