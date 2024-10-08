#ifndef _FPING_H
#define _FPING_H

#define __APPLE_USE_RFC_3542 1

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

/* this requires variadic macros, part of C99 */
#if (defined(DEBUG) || defined(_DEBUG))
extern int64_t current_time_ns;
extern int trace_flag;
#define dbg_printf(fmt, ...) do { if (trace_flag) { fprintf(stderr, "[%10.5f] ", (double)(current_time_ns / 1000)/1000000); fprintf(stderr, fmt, __VA_ARGS__); } } while (0)
            
#else
#define dbg_printf(fmt, ...)
#endif


/* fping.c */
void crash_and_burn( char *message );
void errno_crash_and_burn( char *message );
int in_cksum( unsigned short *p, int n );
extern int random_data_flag;

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
