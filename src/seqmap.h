#ifndef SEQMAP_H
#define SEQMAP_H

#include <sys/time.h>

typedef struct seqmap_value
{
    unsigned int    host_nr;
    unsigned int    ping_count;
    struct timeval  ping_ts;

} SEQMAP_VALUE;

#define SEQMAP_MAXSEQ 65000

#endif
