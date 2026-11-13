#!/bin/sh
set -eu
trap 'rm -f lcov-all.info lcov-filtered.info' EXIT

lcov -c -no-external \
    -d . \
    -b src \
    -o lcov-all.info

lcov --remove lcov-all.info -o lcov-filtered.info \
    '*/optparse.c'

# Normalize runner workspace paths so Coveralls-Finish resolves SF: entries across OSes
PREFIX="SF:$(pwd -P)/" awk '
  BEGIN { p = ENVIRON["PREFIX"]; n = length(p) }
  index($0, p) == 1 { $0 = "SF:" substr($0, n + 1) }
  { print }
' lcov-filtered.info > lcov.info
