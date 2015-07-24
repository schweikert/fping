#ifndef _FPING_H
#define _FPING_H

#define __APPLE_USE_RFC_3542 1

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

#ifndef IPV6
#define FPING_INADDR   struct in_addr
#define FPING_ICMPHDR  struct icmp
#else
#define FPING_INADDR   struct in6_addr
#define FPING_ICMPHDR  struct icmp6_hdr
#endif

/* fping.c */
void crash_and_burn( char *message );
void errno_crash_and_burn( char *message );
int in_cksum( unsigned short *p, int n );

/* socket.c */
int  open_ping_socket();
void socket_set_src_addr(int s, FPING_INADDR src_addr);
int  socket_sendto_ping(int s, struct sockaddr *saddr, socklen_t saddr_len, size_t ping_pkg_size, uint16_t icmp_seq, uint16_t icmp_id);

#endif
