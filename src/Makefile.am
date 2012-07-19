prog =

if IPV4
prog += fping
endif
if IPV6
prog += fping6
endif

sbin_PROGRAMS = ${prog}

fping_SOURCES = fping.c options.h
fping_DEPENDENCIES = ../config.h
fping6_SOURCES = fping.c options.h
fping6_DEPENDENCIES = ../config.h
fping6_CFLAGS = $(AM_CFLAGS) -DIPV6
