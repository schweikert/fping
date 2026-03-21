#!/bin/bash

set -e
set -x

if [[ "$OSTYPE" == "darwin"* ]]; then
    exit 0
fi

# We keep our own list of mirrors because https://ftpmirror.gnu.org is
# unreliable (frequent errors from selected mirror).
MIRRORS=(
    https://mirrors.ocf.berkeley.edu/gnu
    https://mirror.cs.odu.edu/gnu
    https://ftp.gnu.org/gnu
)

AUTOCONF_REL=autoconf/autoconf-2.73.tar.gz
AUTOMAKE_REL=automake/automake-1.18.1.tar.gz
LIBTOOL_REL=libtool/libtool-2.5.4.tar.gz

PREFIX=$(pwd)/ci/build
PATH=$(pwd)/ci/build/bin:$PATH
KEYRING=$(pwd)/ci/fping-deps.gpg

if [ ! -d ci ]; then
    echo "you must run this in the root fping directory" >&2
    exit 1
fi

# remove standard versions
sudo apt-get remove -qq autoconf automake autotools-dev libtool

# install dependencies
sudo apt-get install -y gpgv

# prepare build environment
cd ci
rm -rf build
mkdir -p build/src
cd build/src

mirror_fetch() {
    local relpath="$1"
    for mirror in "${MIRRORS[@]}"; do
        local url="$mirror/$relpath"
        if wget -t 3 "$url"; then
            return 0
        fi
    done
    return 1
}

install_release() {
    local relpath="$1"
    local file=$(basename "$relpath")
    local dir="${file%%.tar.*}"

    if ! mirror_fetch "$relpath"; then
        echo "Failed to download $relpath from any mirror" >&2
        exit 1
    fi

    if ! mirror_fetch "$relpath.sig"; then
        echo "Failed to download $relpath.sig from any mirror" >&2
        exit 1
    fi

    if ! gpgv --keyring "$KEYRING" "$file.sig" "$file"; then
        echo "GPG verification failed for $file"
        exit 1
    fi

    tar xf "$file"
    (
        cd "$dir"
        ./configure --prefix=$PREFIX
        make install
    )
    rm "$file" "$file.sig"
}

# autoconf
install_release $AUTOCONF_REL

# automake
install_release $AUTOMAKE_REL

# libtool
install_release $LIBTOOL_REL
