# Buildbotics CNC Controller Firmware
This repository contains the source code for the Buildbotics CNC Controller.
See [buildbotics.com](https://buildbotics.com/) for more information.

## Overview
![Buildbotics architecture overview](docs/buildbotics_architecture_overview.png)

The main parts of the Buildbotics CNC Controller software and the technologies
they are built with are as follows:

 * Web App - Frontend user interface
   * [Javascript](https://www.w3schools.com/js/)
   * [HTML5](https://www.w3schools.com/html/)
   * [Stylus](http://stylus-lang.com/)
   * [Pug.js](https://pugjs.org/)
   * [Vue.js](https://vuejs.org/)

 * Controller OS - RaspberryPi Operating System
   * [Raspbian](https://www.raspbian.org/)

 * BBCtrl - Python App
   * [Python 3](https://www.python.org/)
   * [Tornado Web](https://www.tornadoweb.org/)

 * GPlan - Path Planner Python Module
   * [C++](http://www.cplusplus.com/)
   * [CAMotics](https://camotics.org/)

 * Main AVR Firmware + Bootloader - Real-time step generation, etc.
   * [ATxmega192a3u](https://www.microchip.com/wwwproducts/ATxmega192A3U)
   * [C](https://en.wikipedia.org/wiki/C_(programming_language))

 * Pwr AVR Firmware - Power safety
   * [ATtiny1634](https://www.microchip.com/wwwproducts/ATtiny1634)
   * [C](https://en.wikipedia.org/wiki/C_(programming_language))

## Quickstart Guide

Be sure to read the [development guide](docs/development.md) for more detailed
instructions.

On a Debian Linux (9.6.0 stable) system:

    # Install the required packages
    sudo apt-get update
    sudo apt-get install -y build-essential git wget binfmt-support qemu \
      parted gcc-avr avr-libc avrdude pylint3 python3 python3-tornado curl \
      unzip python3-setuptools
    curl -sL https://deb.nodesource.com/setup_11.x | sudo -E bash -
    sudo apt-get install -y nodejs

    # Get the source
    git clone https://github.com/buildbotics/bbctrl-firmware

    # Build the Firmware
    cd bbctrl-firmware
    make pkg

The resulting package will be a ``.tar.bz2`` file in ``dist``.  See the
[development guide](docs/development.md) for more information.
