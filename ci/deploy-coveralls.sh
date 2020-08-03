#!/bin/sh

set -xe

pip install --user cpp-coveralls
coveralls --build-root src --exclude src/optparse.c --exclude ci --exclude config.h --gcov-options '\-lp'
