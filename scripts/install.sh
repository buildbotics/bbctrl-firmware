#!/bin/bash

UPDATE_AVR=true
UPDATE_PY=true

while [[ $# -gt 1 ]]; do
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
    ./scripts/avr109-flash.py avr/bbctrl-avr-firmware.hex
fi

if $UPDATE_PY; then
    rm -rf /usr/local/lib/python*/dist-packages/bbctrl-*
    ./setup.py install
    service bbctrl start
fi
