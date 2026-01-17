#ifndef _OUTPUT_H
#define _OUTPUT_H

#include "fping.h"

void print_recv(HOST_ENTRY *h, int64_t recv_time, int result, int this_count, int64_t this_reply, int avg);
void print_timeout(HOST_ENTRY *h, int ping_index);
void print_recv_ext(IP_HEADER_RESULT *ip_header_res, int64_t recv_time, int64_t this_reply);
void print_recv_ext_json(IP_HEADER_RESULT *ip_header_res, int64_t recv_time, int64_t this_reply);
void print_netdata(void);
void print_per_system_splits(void);
void print_per_system_splits_json(void);

const char *sprint_tm(int64_t ns);
void print_timestamp_format(int64_t current_time_ns, int timestamp_format);
#endif
