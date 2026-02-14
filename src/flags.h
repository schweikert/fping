#ifndef FLAGS_H
#define FLAGS_H

#include <stdint.h>

/* sized so as to be like traditional ping */
#define DEFAULT_PING_DATA_SIZE 56

/* options with values */
/* all time-related values are int64_t nanoseconds */
extern unsigned int opt_retry;
extern int64_t opt_timeout;
extern int64_t opt_seqmap_timeout;
extern int64_t opt_interval;
extern int64_t opt_perhost_interval;
extern float opt_backoff;
extern unsigned int opt_ping_data_size;
extern unsigned int opt_count;
extern unsigned int opt_min_reachable;
extern unsigned int opt_ttl;

/* switches 0 = off 1 = on */
extern int opt_print_json_on;
extern int opt_version_on;
extern int opt_verbose_on;
extern int opt_unreachable_on;
extern int opt_alive_on;
extern int opt_quiet_on;
extern int opt_elapsed_on;
extern int opt_stats_on;
extern int opt_cumulative_stats_on;
extern int opt_generate_on;
extern int opt_count_on;
extern int opt_loop_on;
extern int opt_print_netdata_on;
extern int opt_print_json_on;
extern int opt_print_tos_on;
extern int opt_print_ttl_on;
extern int opt_print_srcaddr_on;
extern int opt_per_recv_on;
extern int opt_report_all_rtts_on;
extern int opt_name_on;
extern int opt_addr_on;
extern int opt_rdns_on;
extern int opt_backoff_on;
extern int opt_multif_on;
extern int opt_timeout_on;
extern int opt_fast_reachable_on;
extern int opt_outage_on;
extern int opt_random_data_on;
extern int opt_check_source_on;
extern int opt_size_on;
extern int opt_oiface_on;
extern int opt_bindiface_on;
extern int opt_timestamp_on;
extern int opt_timestamp_format;
extern int opt_icmp_request_typ;

#endif