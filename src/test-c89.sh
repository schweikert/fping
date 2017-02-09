#!/bin/sh

for f in fping.c socket4.c socket6.c seqmap.c; do
    gcc -DHAVE_CONFIG_H -D_BSD_SOURCE -D_POSIX_SOURCE -I.. -Wall -std=c89 -pedantic -c -o /dev/null $f
done
