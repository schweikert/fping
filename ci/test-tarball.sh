#!/bin/bash

# make sure that the .tar.gz file contains everything necessary
# to build fping

set -e
set -x

make dist
VERSION=$(ls fping-*.tar.gz | sed -e 's/^fping-//' | sed -e 's/\.tar\.gz$//')
if [ -z "$VERSION" ]; then
    echo "tar.gz file not found." >&2
    exit 1
fi

# unarchive
TMPDIR=$(mktemp -d --tmpdir=ci)
cd $TMPDIR
tar xf ../fping-$VERSION.tar.gz
DIRNAME=$(ls)

# build
cd $DIRNAME
./configure --enable-ipv4 --enable-ipv6
make
