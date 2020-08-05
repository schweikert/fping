#!/bin/sh

set -e

COVERITY_SCAN_PROJECT_NAME=schweikert/fping
COVERITY_SCAN_EMAIL=david@schweikert.ch

if [ -z "$COVERITY_SCAN_TOKEN" ]; then
    echo "ERROR: COVERITY_SCAN_TOKEN not defined." >&2
    exit 1
fi

curl -o /tmp/cov-analysis-linux64.tgz https://scan.coverity.com/download/linux64 \
    --form project=$COVERITY_SCAN_PROJECT_NAME --form token=$COVERITY_SCAN_TOKEN
tar xfz /tmp/cov-analysis-linux64.tgz
./autogen.sh
./configure --enable-ipv4 --enable-ipv6 --enable-safe-limits --prefix=/opt/fping
cov-analysis-linux64-*/bin/cov-build --dir cov-int make -j4
tar cfz cov-int.tar.gz cov-int
curl https://scan.coverity.com/builds?project=$COVERITY_SCAN_PROJECT_NAME \
    --form token=$COVERITY_SCAN_TOKEN \
    --form email=$COVERITY_SCAN_EMAIL \
    --form file=@cov-int.tar.gz \
    --form version="`git rev-parse --short HEAD`" \
    --form description="`git rev-parse --short HEAD` / $TRAVIS_BUILD_ID "
