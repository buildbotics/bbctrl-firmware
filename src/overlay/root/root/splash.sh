#!/bin/bash

fbi -T 2 -d /dev/fb0 --noverbose -a /root/splash.png

if [ "$1" != noboot ]; then
  exec /sbin/init "$@"
fi
