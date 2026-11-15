AM_CFLAGS = $(WARN_CFLAGS)

FPING_CORE_FILES = fping.c flags.c output.c stats.c seqmap.c socket4.c fping.h flags.h options.h output.h stats.h seqmap.h optparse.c optparse.h
if IPV6
FPING_CORE_FILES += socket6.c
endif

sbin_PROGRAMS = fping
fping_SOURCES = main.c $(FPING_CORE_FILES)
fping_DEPENDENCIES = ../config.h

# unit testing
noinst_HEADERS = greatest.h
check_PROGRAMS = output_test socket4_test fping_test stats_test
TESTS = $(check_PROGRAMS)
output_test_SOURCES = output_test.c $(FPING_CORE_FILES)
output_test_DEPENDENCIES = ../config.h
socket4_test_SOURCES = socket4_test.c $(FPING_CORE_FILES)
socket4_test_DEPENDENCIES = ../config.h
fping_test_SOURCES = fping_test.c $(FPING_CORE_FILES)
fping_test_DEPENDENCIES = ../config.h
stats_test_SOURCES = stats_test.c $(FPING_CORE_FILES)
stats_test_DEPENDENCIES = ../config.h
