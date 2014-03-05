#!/bin/sh

sudo pip install cpp-coveralls --use-mirrors

cd src
gcov *.c
cd ..
coveralls --exclude ci --no-gcov
