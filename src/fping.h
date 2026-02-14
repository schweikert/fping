#ifndef _FPING_H
#define _FPING_H

#define __APPLE_USE_RFC_3542 1

#include <stdint.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <inttypes.h>
#include <sys/time.h>
#include <netdb.h>

/* Structure definitions */
typedef struct ip_header_result {
    int tos;
    int ttl;
    uint32_t otime_ms;
    uint32_t rtime_ms;
    uint32_t ttime_ms;
    char src_addr[INET6_ADDRSTRLEN];
} IP_HEADER_RESULT;

typedef struct host_entry {
    int i; /* index into array */
    char *name; /* name as given by user */
    char *host; /* text description of host */
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
    int64_t *resp_times; /* individual response times */

    /* to avoid allocating two struct events each time that we send a ping, we
     * preallocate here two struct events for each ping that we might send for
     * this host. */
    struct event *event_storage_ping;
    struct event *event_storage_timeout;
} HOST_ENTRY;

struct event {
    HOST_ENTRY *host; /* pointer to associated host */
    int ping_index; /* index/sequence for this ping within the host */
    int64_t ev_time; /* time of the event */
    struct event *ev_next; /* next event in linked list */
    struct event *ev_prev; /* previous event in linked list */
};

struct event_queue {
    struct event *first;
    struct event *last;
};

/* Global variables */
extern HOST_ENTRY **table;
extern int num_hosts;
extern int max_hostname_len;
extern int64_t current_time_ns;
extern struct timespec current_time;
extern int64_t start_time;
extern int64_t end_time;
extern int64_t opt_perhost_interval;
extern int64_t report_interval;

// Stats globals
extern int num_alive, num_unreachable, num_noaddress, num_timeout;
extern int num_pingsent, num_pingreceived, num_othericmprcvd;
extern int64_t max_reply, min_reply, total_replies, sum_replies;


/* this requires variadic macros, part of C99 */
#if (defined(DEBUG) || defined(_DEBUG))
extern int opt_debug_trace_on;
#define dbg_printf(fmt, ...) do { if (opt_debug_trace_on) { fprintf(stderr, "[%10.5f] ", (double)(current_time_ns / 1000)/1000000); fprintf(stderr, fmt, __VA_ARGS__); } } while (0)
            
#else
#define dbg_printf(fmt, ...)
#endif

/* fping.c */
void add_name(char *name);
void add_addr(char *name, char *host, struct sockaddr *ipaddr, socklen_t ipaddr_len);
char *na_cat(char *name, struct in_addr ipaddr);
char *get_host_by_address(struct in_addr in);
int send_ping(HOST_ENTRY *h, int index);
void usage(int);
int wait_for_reply(int64_t);
void stats_reset_interval(HOST_ENTRY *h);
void main_loop();
void signal_handler(int);
void finish();
const char *sprint_tm(int64_t t);
void ev_enqueue(struct event_queue *queue, struct event *event);
struct event *ev_dequeue(struct event_queue *queue);
void ev_remove(struct event_queue *queue, struct event *event);
void add_cidr(char *);
void add_cidr_ipv4(unsigned long, unsigned long);
void add_range(char *, char *);
void add_addr_range_ipv4(unsigned long, unsigned long);
#ifdef IPV6
uint64_t be_octets_to_uint64(uint8_t*);
void uint64_to_be_octets(uint64_t, uint8_t*);
void add_cidr_ipv6(uint64_t, uint64_t, unsigned long, const char *);
void add_addr_range_ipv6(uint64_t, uint64_t, uint64_t, uint64_t, const char *);
#endif
void print_warning(char *fmt, ...);
int addr_cmp(struct sockaddr *a, struct sockaddr *b);
void host_add_ping_event(HOST_ENTRY *h, int index, int64_t ev_time);
void host_add_timeout_event(HOST_ENTRY *h, int index, int64_t ev_time);
struct event *host_get_timeout_event(HOST_ENTRY *h, int index);
void stats_add(HOST_ENTRY *h, int index, int success, int64_t latency);
void update_current_time();
void print_timestamp_format(int64_t current_time_ns, int timestamp_format);
void crash_and_burn( char *message );
void errno_crash_and_burn( char *message );
int in_cksum( unsigned short *p, int n );

/* socket.c */
int  open_ping_socket_ipv4(int *socktype);
void socket_set_outgoing_iface_ipv4(int s, const char *iface_name);
void init_ping_buffer_ipv4(size_t ping_data_size);
void socket_set_src_addr_ipv4(int s, struct in_addr *src_addr, int *ident);
int  socket_sendto_ping_ipv4(int s, struct sockaddr *saddr, socklen_t saddr_len, uint16_t icmp_seq, uint16_t icmp_id, uint8_t icmp_proto);
#ifdef IPV6
int  open_ping_socket_ipv6(int *socktype);
void socket_set_outgoing_iface_ipv6(const char *iface_name);
void init_ping_buffer_ipv6(size_t ping_data_size);
void socket_set_src_addr_ipv6(int s, struct in6_addr *src_addr, int *ident);
int  socket_sendto_ping_ipv6(int s, struct sockaddr *saddr, socklen_t saddr_len, uint16_t icmp_seq, uint16_t icmp_id);
#endif

#endif
