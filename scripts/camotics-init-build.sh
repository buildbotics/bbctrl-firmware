#!/bin/bash -ex

IMG_DATE=2017-11-29
IMG_BASE=${IMG_DATE}-raspbian-stretch-lite
BASE_URL=https://downloads.raspberrypi.org/raspbian_lite/images
IMG_URL=$BASE_URL/raspbian_lite-2017-12-01/$IMG_BASE.zip
CAMOTICS_IMG=camotics-dev.img

# Create dev image
if [ ! -e $CAMOTICS_IMG ]; then

    # Get base image
    if [ ! -e $IMG_BASE.img ]; then
        if [ ! -e $IMG_BASE.zip ]; then
            wget $IMG_URL
        fi

        unzip $IMG_BASE.zip
    fi

    # Copy base image
    cp $IMG_BASE.img $CAMOTICS_IMG.tmp

    # Init image
    mkdir -p rpi-share
    cp ./scripts/camotics-init-dev-img.sh rpi-share
    sudo ./scripts/rpi-chroot.sh $CAMOTICS_IMG.tmp /mnt/host/camotics-init-dev-img.sh

    # Move image
    mv $CAMOTICS_IMG.tmp $CAMOTICS_IMG
fi

# Get repos
function fetch_local_repo() {
    mkdir -p $1
    git -C $1 init
    git -C $1 fetch -t "$2" $3
    git -C $1 reset --hard FETCH_HEAD
}

mkdir -p rpi-share || true

if [ ! -e rpi-share/cbang ]; then
    if [ "$CBANG_HOME" != "" ]; then
        fetch_local_repo rpi-share/cbang "$CBANG_HOME" master
    else
        git clone https://github.com/CauldronDevelopmentLLC/cbang \
            rpi-share/cbang
    fi
fi

if [ ! -e rpi-share/camotics ]; then
    if [ "$CAMOTICS_HOME" != "" ]; then
        fetch_local_repo rpi-share/camotics "$CAMOTICS_HOME" master
    else
        git clone https://github.com/CauldronDevelopmentLLC/camotics \
            rpi-share/camotics
    fi
fi
