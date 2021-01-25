#ifndef _FPING_H
#define _FPING_H

#define __APPLE_USE_RFC_3542 1

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdio.h>
#include <time.h>

/* this requires variadic macros, part of C99 */
#if (defined(DEBUG) || defined(_DEBUG))
extern int64_t current_time_ns;
extern int trace_flag;
#define dbg_printf(fmt, ...) do { if (trace_flag) { fprintf(stderr, "[%10.5f] ", (double)(current_time_ns / 1000)/1000000); fprintf(stderr, fmt, __VA_ARGS__); } } while (0)
            
#else
#define dbg_printf(fmt, ...)
#endif

struct event;
typedef struct host_entry {
    int i; /* index into array */
    char* name; /* name as given by user */
    char* host; /* text description of host */
    struct sockaddr_storage saddr; /* internet address */
    socklen_t saddr_len;
    int64_t timeout; /* time to wait for response */
    int64_t last_send_time; /* time of last packet sent */
    int num_sent; /* number of ping packets sent (for statistics) */
    int num_recv; /* number of pings received (duplicates ignored) */
    int num_recv_total; /* number of pings received, including duplicates */
    int64_t max_reply; /* longest response time */
    int64_t min_reply; /* shortest response time */
    int64_t total_time; /* sum of response times */
    /* _i -> splits (reset on every report interval) */
    int num_sent_i; /* number of ping packets sent */
    int num_recv_i; /* number of pings received */
    int64_t max_reply_i; /* longest response time */
    int64_t min_reply_i; /* shortest response time */
    int64_t total_time_i; /* sum of response times */
    int64_t* resp_times; /* individual response times */

    /* to avoid allocating two struct events each time that we send a ping, we
     * preallocate here two struct events for each ping that we might send for
     * this host. */
    struct event *event_storage_ping;
    struct event *event_storage_timeout;
} HOST_ENTRY;

struct event {
    struct event *ev_prev;
    struct event *ev_next;
    int64_t ev_time;
    struct host_entry *host;
    int ping_index;
};

struct event_queue {
    struct event *first;
    struct event *last;
};


/* fping.c */
void crash_and_burn( char *message );
void errno_crash_and_burn( char *message );
int in_cksum( unsigned short *p, int n );
extern int random_data_flag;

/* socket.c */
int  open_ping_socket_ipv4(int *socktype);
void init_ping_buffer_ipv4(size_t ping_data_size);
void socket_set_src_addr_ipv4(int s, struct in_addr *src_addr, int *ident);
int  socket_sendto_ping_ipv4(int s, struct sockaddr *saddr, socklen_t saddr_len, uint16_t icmp_seq, uint16_t icmp_id);
#ifdef IPV6
int  open_ping_socket_ipv6(int *socktype);
void init_ping_buffer_ipv6(size_t ping_data_size);
void socket_set_src_addr_ipv6(int s, struct in6_addr *src_addr, int *ident);
int  socket_sendto_ping_ipv6(int s, struct sockaddr *saddr, socklen_t saddr_len, uint16_t icmp_seq, uint16_t icmp_id);
#endif

/* stats.c */
/* global stats */
extern int64_t max_reply;
extern int64_t min_reply;
extern int64_t total_replies;
extern int64_t sum_replies;
extern int max_hostname_len;
extern int num_hosts; /* total number of hosts */
extern int num_alive, /* total number alive */
    num_unreachable, /* total number unreachable */
    num_noaddress; /* total number of addresses not found */
extern int num_timeout, /* number of times select timed out */
    num_pingsent, /* total pings sent */
    num_pingreceived, /* total pings received */
    num_othericmprcvd; /* total non-echo-reply ICMP received */

extern int verbose_flag, quiet_flag, stats_flag, unreachable_flag, alive_flag;
extern int elapsed_flag, version_flag, count_flag, loop_flag, netdata_flag;
extern int per_recv_flag, report_all_rtts_flag, name_flag, addr_flag, backoff_flag, rdns_flag;
extern int multif_flag, timeout_flag;
extern int outage_flag;
extern HOST_ENTRY** table;
extern int64_t perhost_interval;
extern int64_t report_interval;
extern struct timespec current_time;
extern int64_t start_time;
extern int64_t end_time;
void print_per_system_stats(void);
void print_per_system_splits(void);
void print_netdata(void);
void print_global_stats(void);
void stats_add(HOST_ENTRY *h, int index, int success, int64_t latency);
const char* sprint_tm(int64_t t);
void update_current_time();

#endif
