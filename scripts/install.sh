#!/bin/bash

sudo service bbctrl stop
sudo ./scripts/avr109-flash.py avr/bbctrl-avr-firmware.hex
sudo rm -rf /usr/local/lib/python*/dist-packages/bbctrl-*
sudo ./setup.py install
sudo service bbctrl start
