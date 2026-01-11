#ifndef _FPING_H
#define _FPING_H

#define __APPLE_USE_RFC_3542 1

#include <stdint.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

/* this requires variadic macros, part of C99 */
#if (defined(DEBUG) || defined(_DEBUG))
extern int64_t current_time_ns;
extern int opt_debug_trace_on;
#define dbg_printf(fmt, ...) do { if (opt_debug_trace_on) { fprintf(stderr, "[%10.5f] ", (double)(current_time_ns / 1000)/1000000); fprintf(stderr, fmt, __VA_ARGS__); } } while (0)
            
#else
#define dbg_printf(fmt, ...)
#endif


/* fping.c */
typedef struct host_entry HOST_ENTRY;
struct event;
struct event_queue;
typedef struct ip_header_result IP_HEADER_RESULT;
extern int opt_random_data_on;

void add_name(char *name);
void add_addr(char *name, char *host, struct sockaddr *ipaddr, socklen_t ipaddr_len);
char *na_cat(char *name, struct in_addr ipaddr);
char *get_host_by_address(struct in_addr in);
int send_ping(HOST_ENTRY *h, int index);
void usage(int);
int wait_for_reply(int64_t);
void print_recv(HOST_ENTRY *h, int64_t recv_time, int result, int this_count, int64_t this_reply, int avg);
void print_timeout(HOST_ENTRY *h, int ping_index);
void print_recv_ext(IP_HEADER_RESULT *ip_header_res, int64_t recv_time, int64_t this_reply);
void print_recv_ext_json(IP_HEADER_RESULT *ip_header_res, int64_t recv_time, int64_t this_reply);
void print_per_system_stats(void);
void print_per_system_stats_json(void);
void print_per_system_splits(void);
void print_per_system_splits_json(void);
void stats_reset_interval(HOST_ENTRY *h);
void print_netdata(void);
void print_global_stats(void);
void print_global_stats_json(void);
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
void init_ping_buffer_ipv4(size_t ping_data_size);
void socket_set_src_addr_ipv4(int s, struct in_addr *src_addr, int *ident);
int  socket_sendto_ping_ipv4(int s, struct sockaddr *saddr, socklen_t saddr_len, uint16_t icmp_seq, uint16_t icmp_id, uint8_t icmp_proto);
#ifdef IPV6
int  open_ping_socket_ipv6(int *socktype);
void init_ping_buffer_ipv6(size_t ping_data_size);
void socket_set_src_addr_ipv6(int s, struct in6_addr *src_addr, int *ident);
int  socket_sendto_ping_ipv6(int s, struct sockaddr *saddr, socklen_t saddr_len, uint16_t icmp_seq, uint16_t icmp_id);
#endif

#endif
