#!/bin/bash

set -e
set -x

if [[ "$OSTYPE" == "darwin"* ]]; then
    exit 0
fi

# We keep our own list of mirrors because https://ftpmirror.gnu.org is
# unreliable (frequent errors from selected mirror).
MIRRORS=(
    https://mirror.cs.odu.edu/gnu
    https://mirrors.ocf.berkeley.edu/gnu
    https://ftp.gnu.org/gnu
)

AUTOCONF_REL=autoconf/autoconf-2.72.tar.gz
AUTOMAKE_REL=automake/automake-1.18.1.tar.gz
LIBTOOL_REL=libtool/libtool-2.5.4.tar.gz

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

install_release() {
    local relpath=$1
    local file=$(basename "$relpath")
    local dir="${file%%.tar.*}"

    local success=0
    for mirror in "${MIRRORS[@]}"; do
        local url="$mirror/$relpath"
        if wget -t 3 -O "$file" "$url"; then
            success=1
            break
        fi
    done

    if [ $success -eq 0 ]; then
        echo "Failed to download $relpath from any mirror" >&2
        exit 1
    fi

    tar xf "$file"
    (
        cd "$dir"
        ./configure --prefix=$PREFIX
        make install
    )
    rm "$file"
}

# autoconf
install_release $AUTOCONF_REL

# automake
install_release $AUTOMAKE_REL

# libtool
install_release $LIBTOOL_REL
