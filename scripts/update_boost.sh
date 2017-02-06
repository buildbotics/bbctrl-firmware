#!/bin/bash -e

VERSION=1.63.0
NAME=boost_$(echo $VERSION | tr . _)
PKG=$NAME.7z
URL=https://sf.net/projects/boost/files/boost/$VERSION/$PKG
BCP=./$NAME/dist/bin/bcp

# Get boost
if [ ! -d $NAME ]; then
    if [ ! -e $PKG ]; then wget $URL; fi
    7z x -y $PKG
fi

# Build bcp
if [ ! -e $BCP ]; then
    pushd $NAME
    ./bootstrap.sh
    ./b2 tools/bcp
    popd
fi

# Extract files
$BCP --scan --boost=$NAME --unix-lines $(
    find src/cbang -name \*.h -o -name \*.cpp
) src/boost

# Remove uneeded files
for PATH in Jamroot libs/config libs/date_time libs/smart_ptr; do
    /bin/rm -rf src/boost/$PATH
done

# Done
echo ok
