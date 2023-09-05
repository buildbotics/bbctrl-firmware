#!/bin/bash

DISK=mmcblk0
PATH=$PATH:/usr/sbin

if [ -e /root/resize2 ]; then
  mount /dev/${DISK}p1 /boot
  sed -i 's|init=/root/resize.sh||' /boot/cmdline.txt
  umount /boot

  mount -t proc proc /proc
  mount -t sysfs sys /sys
  mount -o remount,rw /

  resize2fs -f /dev/${DISK}p2

  rm /root/resize.sh /root/resize2

  exec /sbin/init "$@"

else
  mount -o remount,rw /
  touch /root/resize2

  printf "w\ny\ny\n" | gdisk /dev/$DISK
  parted -s /dev/$DISK -- unit s resizepart 2 -34s
fi

mount -o remount,ro /
reboot -f
