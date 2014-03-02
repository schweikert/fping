#!/bin/sh

# upload to bintray.com/schweikert

VERSION=3.9-rc1
curl -T fping-$VERSION.tar.gz -uschweikert:$BINTRAY_API_KEY https://api.bintray.com/content/schweikert/generic/fping-rc/$VERSION/fping-$VERSION.tar.gz
