#!/bin/bash

DISK=mmcblk0
PATH=$PATH:/usr/sbin

mount -t proc proc /proc
mount -t sysfs sys /sys
mount -o remount,rw /

if [ -e /root/resize2 ]; then
  mount /dev/${DISK}p1 /boot
  sed -i 's|/resize.sh|/splash.sh|' /boot/cmdline.txt
  umount /boot

  echo Resizing filesystem
  resize2fs -f /dev/${DISK}p2

  rm /root/resize.sh /root/resize2

  /root/splash.sh

else
  touch /root/resize2

  echo Resizing partition
  printf "w\ny\ny\n" | gdisk /dev/$DISK
  parted -s /dev/$DISK -- unit s resizepart 2 -34s
fi

mount -o remount,ro /

# Force a reboot
echo 1 > /proc/sys/kernel/sysrq
echo b > /proc/sysrq-trigger
reboot -f
