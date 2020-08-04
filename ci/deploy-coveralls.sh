#!/bin/sh

set -xe

if [ "$TRAVIS_DIST" = "trusty" ]; then
    echo "skip coveralls on trusty because coveralls errors out due to python issues"
    exit 0
elif [ "$TRAVIS_OS_NAME" = "osx" ]; then
    pip3 install --user cpp-coveralls
    PATH="${PATH}:$(python3 -c 'import site; print(site.USER_BASE)')/bin"
else
    pip install --user cpp-coveralls
fi

export COVERALLS_PARALLEL=true
coveralls --build-root src --exclude src/optparse.c --exclude ci --exclude config.h --gcov-options '\-lp'
