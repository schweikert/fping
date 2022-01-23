#!/bin/sh

lcov -c -no-external \
    -d . \
    -b src \
    -o lcov-all.info

lcov --remove lcov-all.info -o lcov.info \
    '*/optparse.c'
