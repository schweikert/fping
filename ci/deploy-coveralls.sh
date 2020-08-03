#!/bin/sh

set -xe

pip install --user cpp-coveralls
coveralls --root src --exclude src/optparse.c --gcov-options '\-lp'
