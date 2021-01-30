#!/bin/bash -ex

cd /mnt/host
export V8_INCLUDE=/opt/embedded-v8/include/ V8_LIBPATH=/opt/embedded-v8/out/
scons -C cbang disable_local="re2 libevent"
export CBANG_HOME="/mnt/host/cbang"
scons -C camotics build/camotics.so with_gui=0
