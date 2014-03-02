#!/bin/bash

# upload to bintray.com/schweikert

make dist
VERSION=$(ls fping-*.tar.gz | sed -e 's/^fping-//' | sed -e 's/\.tar\.gz$//')
curl -T fping-$VERSION.tar.gz -uschweikert:$BINTRAY_API_KEY https://api.bintray.com/content/schweikert/generic/fping-rc/$VERSION/fping-$VERSION.tar.gz
