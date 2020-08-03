#!/bin/sh

set -ex

PATH=`pwd`/src:$PATH

prove ci/test-*.pl
ci/test-tarball.sh
