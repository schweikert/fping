#!/bin/sh
[ -f Makefile ] && make clean
rm -f Makefile
rm -f Makefile.in
rm -f aclocal.m4
rm -rf autom4te.cache
rm -f config.guess
rm -f config.h
rm -f config.h.in
rm -f config.log
rm -f config.status
rm -f config.sub
rm -f configure
rm -f depcomp
rm -f install-sh
rm -f missing
rm -f mkinstalldirs
rm -f stamp-h1
rm -f doc/Makefile.in
rm -f src/Makefile.in
rm -f doc/fping.8
