#!/bin/sh

set -ex

PATH=`pwd`/src:$PATH

make check VERBOSE=1
prove ci/test-*.pl
ci/test-tarball.sh
