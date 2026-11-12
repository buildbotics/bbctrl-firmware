#!/bin/bash

export LC_ALL=C
export DEBIAN_FRONTEND=noninteractive

# Install packages
apt-get install -yq git build-essential libx11-dev libxft-dev libxtst-dev \
  libxinerama-dev libxcursor-dev libfontconfig-dev libssl-dev libnode-dev scons \
  python3-dev python3-distutils

if [ -e /proc/cpuinfo ]; then
  CPUS=$(grep -c ^processor /proc/cpuinfo)
else
  CPUS=24
fi

export CBANG_HOME=$PWD/cbang

PROJECTS="buildbotics/updiprog buildbotics/rpipdi buildbotics/bbkbd "
PROJECTS+="cauldrondevelopmentllc/cbang cauldrondevelopmentllc/camotics"

# Upstream commits that patches/camotics/ was developed and checked against.
# Projects not listed here build from origin/master.  Change a pin on purpose,
# then check the patches still apply and behave.
declare -A PIN=(
  [cbang]=14146609d60915b65b7c4b38dd36b9b6505e7bfa
  [camotics]=e84665f2fa9d1151f03282ac7e01320bc65e015b
)

for PROJECT in $PROJECTS; do
  NAME=$(basename $PROJECT)
  if [ ! -e $NAME ]; then
    git clone https://github.com/$PROJECT || exit 1
  else
    git -C $NAME fetch || exit 1
  fi
  git -C $NAME reset --hard ${PIN[$NAME]:-origin/master} || exit 1

  if [ $NAME == cbang ]; then
    scons -C cbang -j$CPUS v8_compress_pointers=0

  elif [ $NAME == camotics ]; then
    # Local changes on top of upstream, see patches/camotics/.  Reapplied on
    # every build because the reset above discards them.
    for PATCH in /root/patches/camotics/*.patch; do
      git -C camotics apply "$PATCH" || exit 1
    done
    scons -C camotics -j$CPUS build/camotics.so with_gui=0 wrap_glibc=0
  else
    make -C $NAME -j$CPUS
  fi
done
