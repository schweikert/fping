#ifndef SEQMAP_H
#define SEQMAP_H

#include <sys/time.h>

typedef struct seqmap_value
{
    unsigned int    host_nr;
    unsigned int    ping_count;
    struct timeval  ping_ts;

} SEQMAP_VALUE;

#define SEQMAP_MAXSEQ 65535

void seqmap_init();
unsigned int seqmap_add(unsigned int host_nr, unsigned int ping_count, struct timeval *now);
SEQMAP_VALUE *seqmap_fetch(unsigned int id, struct timeval *now);
void seqmap_clear(unsigned int id);
unsigned int seqmap_get_oldest_id();

#endif
