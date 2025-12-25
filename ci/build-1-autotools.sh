#!/bin/bash

set -e
set -x

if [[ "$OSTYPE" == "darwin"* ]]; then
    exit 0
fi

AUTOCONF=https://ftpmirror.gnu.org/autoconf/autoconf-2.72.tar.gz
AUTOMAKE=https://ftpmirror.gnu.org/automake/automake-1.18.1.tar.gz
LIBTOOL=https://ftpmirror.gnu.org/libtool/libtool-2.5.4.tar.gz
PREFIX=$(pwd)/ci/build
PATH=$(pwd)/ci/build/bin:$PATH

if [ ! -d ci ]; then
    echo "you must run this in the root fping directory" >&2
    exit 1
fi

# remove standard versions
sudo apt-get remove -qq autoconf automake autotools-dev libtool

# prepare build environment
cd ci
rm -rf build
mkdir -p build/src
cd build/src

# autoconf
(
AUTOCONF_FILE=$(basename $AUTOCONF)
AUTOCONF_DIR=$(echo $AUTOCONF_FILE | sed -e 's/\.tar.*//')
wget -t 5 --retry-connrefused --waitretry=5 $AUTOCONF
tar xf $AUTOCONF_FILE
cd $AUTOCONF_DIR
./configure --prefix=$PREFIX
make install
)

# automake
(
AUTOMAKE_FILE=$(basename $AUTOMAKE)
AUTOMAKE_DIR=$(echo $AUTOMAKE_FILE | sed -e 's/\.tar.*//')
wget -t 5 --retry-connrefused --waitretry=5 $AUTOMAKE
tar xf $AUTOMAKE_FILE
cd $AUTOMAKE_DIR
./configure --prefix=$PREFIX
make install
)

# libtool
(
LIBTOOL_FILE=$(basename $LIBTOOL)
LIBTOOL_DIR=$(echo $LIBTOOL_FILE | sed -e 's/\.tar.*//')
wget -t 5 --retry-connrefused --waitretry=5 $LIBTOOL
tar xf $LIBTOOL_FILE
cd $LIBTOOL_DIR
./configure --prefix=$PREFIX
make install
)
