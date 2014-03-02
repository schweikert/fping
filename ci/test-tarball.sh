#!/bin/bash

# make sure that the .tar.gz file contains everything necessary
# to build fping

set -e
set -x

TARFILE=fping-*.tar.gz
if [ ! -f "$TARFILE" ]; then
    echo "tar.gz file not found." >&2
    exit 1
fi

# unarchive
TMPDIR=$(mktemp -d --tmpdir=.)
cd $TMPDIR
tar xf ../$TARFILE
DIRNAME=$(ls)

# build
cd $DIRNAME
./configure --enable-ipv4 --enable-ipv6
make
