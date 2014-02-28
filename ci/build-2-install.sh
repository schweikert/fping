#!/bin/bash -e

PATH=$(pwd)/ci/build/bin:$PATH

if [ ! -d ci ]; then
    echo "you must run this in the root fping directory" >&2
    exit 1
fi

autoreconf -i
./configure --enable-ipv4 --enable-ipv6 --prefix=/opt/fping
make 
sudo make install
