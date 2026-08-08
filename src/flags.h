#ifndef FLAGS_H
#define FLAGS_H

#include <stdint.h>

/* sized so as to be like traditional ping */
#define DEFAULT_PING_DATA_SIZE 56

typedef struct {
    /* options with values */
    /* all time-related values are int64_t nanoseconds */
    unsigned int retry;
    int64_t timeout;
    int64_t seqmap_timeout;
    int64_t interval;
    int64_t perhost_interval;
    float backoff;
    unsigned int ping_data_size;
    unsigned int count;
    unsigned int min_reachable;
    unsigned int ttl;

    /* switches: 0 = off, 1 = on */
    int version_on;
    int verbose_on;
    int unreachable_on;
    int alive_on;
    int quiet_on;
    int elapsed_on;
    int stats_on;
    int cumulative_stats_on;
    int generate_on;
    int count_on;
    int loop_on;
    int print_netdata_on;
    int print_json_on;
    int print_tos_on;
    int print_ttl_on;
    int print_reply_dst_on;
    int per_recv_on;
    int report_all_rtts_on;
    int name_on;
    int addr_on;
    int rdns_on;
    int backoff_on;
    int multif_on;
    int timeout_on;
    int fast_reachable_on;
    int outage_on;
    int random_data_on;
    int check_source_on;
    int size_on;
    int oiface_on;
    int bindiface_on;
    int timestamp_on;
    int timestamp_format;
    int icmp_request_typ;
} Options;

typedef struct {
    /* debug switches */
    int trace_on;
    int randomly_lose_on;
    int print_per_system_on;
    int lose_factor;
} DebugOptions;


Options options_init(void);
DebugOptions debug_options_init(void);

extern Options opt;
extern DebugOptions dbg_opt;

#endif