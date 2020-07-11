#ifndef SEQMAP_H
#define SEQMAP_H

#include <sys/time.h>

typedef struct seqmap_value
{
    unsigned int    host_nr;
    unsigned int    ping_count;
    struct timespec ping_ts;

} SEQMAP_VALUE;

#define SEQMAP_MAXSEQ 65535

void seqmap_init();
unsigned int seqmap_add(unsigned int host_nr, unsigned int ping_count, struct timespec *now);
SEQMAP_VALUE *seqmap_fetch(unsigned int id, struct timespec *now);

#endif
