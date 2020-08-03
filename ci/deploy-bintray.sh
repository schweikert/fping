#!/bin/bash

# upload to bintray.com/schweikert

set -e

VERSION=$(ls fping-*.tar.gz | sed -e 's/^fping-//' | sed -e 's/\.tar\.gz$//')
if [[ "$VERSION" =~ ^[0-9]+\.[0-9]+$ ]]; then
    REPO=release
else
    REPO=beta
fi
curl -T fping-$VERSION.tar.gz -uschweikert:$BINTRAY_API_KEY https://api.bintray.com/content/schweikert/$REPO/fping/$VERSION/fping-$VERSION.tar.gz
echo
