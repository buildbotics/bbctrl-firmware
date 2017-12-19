#!/bin/bash -ex

cd /mnt/host
scons -C cbang disable_local="re2 libevent"
export CBANG_HOME="/mnt/host/cbang"
scons -C camotics gplan.so with_gui=0 with_tpl=0
