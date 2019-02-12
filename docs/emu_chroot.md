This document describes how to setup the Buildbotics firmware in a chroot
environment for the purposes of demonstrating the user interface.

On a Debian system install:

    ROOT=/opt/demo
    sudo apt-get install -y binutils debootstrap
    sudo mkdir $ROOT
    sudo debootstrap --arch amd64 stable $ROOT/ http://deb.debian.org/debian

Then chroot:

    sudo mount --bind /dev $ROOT/dev/
    sudo mount --bind /sys $ROOT/sys/
    sudo mount --bind /proc $ROOT/proc/
    sudo mount --bind /dev/pts $ROOT/dev/pts
    sudo chroot $ROOT

Setup the demo system:

    export LC_ALL=C
    apt-get update
    apt-get install -y wget git python3-tornado python3-sockjs-tornado \
      python3-setuptools python-six build-essential scons libv8-dev
      libpython3-dev

    cd /opt
    BASE=https://buildbotics.com/bbctrl
    LATEST=$(wget $BASE/latest.txt -O- -q)
    wget $BASE/bbctrl-$LATEST.tar.bz2
    tar xf bbctrl-$LATEST.tar.bz2
    ln -sf bbctrl-$LATEST bbctrl

    git clone --depth=1 https://github.com/CauldronDevelopmentLLC/cbang
    git clone --depth=1 https://github.com/CauldronDevelopmentLLC/camotics
    export CBANG_HOME=/opt/cbang
    scons -C cbang -j8 disable_local="re2 libevent"
    scons -C camotics -j8 gplan.so with_gui=False

    cd bbctrl
    python3 setup.py install
    cp /opt/camotics/gplan.so /usr/local/lib/python*/dist-packages/bbctrl-$VERSION-py*.egg/camotics/gplan.so

    mkdir -p /var/lib/bbctrl/upload
    useradd -u 1001 bbmc
