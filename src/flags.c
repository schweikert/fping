#include "flags.h"
#include "options.h"

Options options_init(void)
{
    Options o = { 0 };

    /* options with values — set non-zero defaults */
    o.retry             = DEFAULT_RETRY;
    o.timeout           = (int64_t)DEFAULT_TIMEOUT * 1000000;
    o.seqmap_timeout    = (int64_t)DEFAULT_SEQMAP_TIMEOUT * 1000000;
    o.interval          = (int64_t)DEFAULT_INTERVAL * 1000000;
    o.perhost_interval  = (int64_t)DEFAULT_PERHOST_INTERVAL * 1000000;
    o.backoff           = DEFAULT_BACKOFF_FACTOR;
    o.ping_data_size    = DEFAULT_PING_DATA_SIZE;
    o.count             = 1;

    return o;
}

DebugOptions debug_options_init(void)
{
    DebugOptions o = { 0 };
  
    return o;
}

Options opt;
DebugOptions dbg_opt;