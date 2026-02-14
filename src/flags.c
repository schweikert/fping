#include "flags.h"
#include "options.h"

#include <stdint.h>

/* options with values */
/* all time-related values are int64_t nanoseconds */
unsigned int opt_retry = DEFAULT_RETRY;
int64_t opt_timeout = (int64_t)DEFAULT_TIMEOUT * 1000000;
int64_t opt_seqmap_timeout = (int64_t)DEFAULT_SEQMAP_TIMEOUT * 1000000;
int64_t opt_interval = (int64_t)DEFAULT_INTERVAL * 1000000;
int64_t opt_perhost_interval = (int64_t)DEFAULT_PERHOST_INTERVAL * 1000000;
float opt_backoff = DEFAULT_BACKOFF_FACTOR;
unsigned int opt_ping_data_size = DEFAULT_PING_DATA_SIZE;
unsigned int opt_count = 1;
unsigned int opt_min_reachable = 0;
unsigned int opt_ttl = 0;

/* switches 0 = off 1 = on */
int opt_version_on = 0;
int opt_verbose_on = 0;
int opt_unreachable_on = 0;
int opt_alive_on = 0;
int opt_quiet_on = 0;
int opt_elapsed_on = 0;
int opt_stats_on = 0;
int opt_cumulative_stats_on = 0;
int opt_generate_on = 0; /* flag for IP list generation */
int opt_count_on = 0;
int opt_loop_on = 0;
int opt_print_netdata_on = 0;
int opt_print_json_on = 0;
int opt_print_tos_on = 0;
int opt_print_ttl_on = 0;
int opt_print_srcaddr_on = 0;
int opt_per_recv_on = 0;
int opt_report_all_rtts_on = 0;
int opt_name_on = 0;
int opt_addr_on = 0;
int opt_rdns_on = 0;
int opt_backoff_on = 0;
int opt_multif_on = 0;
int opt_timeout_on = 0;
int opt_fast_reachable_on = 0;
int opt_outage_on = 0;
int opt_random_data_on = 0;
int opt_check_source_on = 0;
int opt_size_on = 0;
int opt_oiface_on = 0;
int opt_bindiface_on = 0;
int opt_timestamp_on = 0;
int opt_timestamp_format = 0;
int opt_icmp_request_typ = 0;