#!/bin/bash
set -x

PATH=$(pwd)/ci/build/bin:$PATH

if [ ! -d ci ]; then
    echo "you must run this in the root fping directory" >&2
    exit 1
fi

autoreconf -i
./configure --enable-ipv4 --enable-ipv6 --enable-safe-limits --prefix=/opt/fping
make CFLAGS="-g -O0 -fprofile-arcs -ftest-coverage"

# use setuid, since setcap is not available
sudo chown root src/fping
sudo chmod u+s  src/fping
