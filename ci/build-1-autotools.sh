#!/bin/bash -e

if [[ "$OSTYPE" == "darwin"* ]]; then
    exit 0
fi

AUTOCONF=http://ftp.gnu.org/gnu/autoconf/autoconf-2.69.tar.gz
AUTOMAKE=http://ftp.gnu.org/gnu/automake/automake-1.14.1.tar.gz
LIBTOOL=http://alpha.gnu.org/gnu/libtool/libtool-2.4.2.418.tar.gz
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
wget $AUTOCONF
tar xf $AUTOCONF_FILE
cd $AUTOCONF_DIR
./configure --prefix=$PREFIX
make install
)

# automake
(
AUTOMAKE_FILE=$(basename $AUTOMAKE)
AUTOMAKE_DIR=$(echo $AUTOMAKE_FILE | sed -e 's/\.tar.*//')
wget $AUTOMAKE
tar xf $AUTOMAKE_FILE
cd $AUTOMAKE_DIR
./configure --prefix=$PREFIX
make install
)

# libtool
(
LIBTOOL_FILE=$(basename $LIBTOOL)
LIBTOOL_DIR=$(echo $LIBTOOL_FILE | sed -e 's/\.tar.*//')
wget $LIBTOOL
tar xf $LIBTOOL_FILE
cd $LIBTOOL_DIR
./configure --prefix=$PREFIX
make install
)
