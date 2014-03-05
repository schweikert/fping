#!/bin/sh

set -x

sudo pip install cpp-coveralls --use-mirrors

cd src

ls -l

gcov *.o
cd ..
coveralls --exclude ci --no-gcov
