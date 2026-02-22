/*
 * fping: fast-ping, file-ping, favorite-ping, funky-ping
 *
 *   Ping a list of target hosts in a round robin fashion.
 *   A better ping overall.
 *
 * fping website:  http://www.fping.org
 *
 * Current maintainer of fping: David Schweikert
 * Please send suggestions and patches to: david@schweikert.ch
 *
 *
 * Original author:  Roland Schemers  <schemers@stanford.edu>
 * IPv6 Support:     Jeroen Massar    <jeroen@unfix.org / jeroen@ipng.nl>
 * Improved main loop: David Schweikert <david@schweikert.ch>
 * Debian Merge, TOS settings: Tobi Oetiker <tobi@oetiker.ch>
 * Bugfixes, byte order & senseful seq.-numbers: Stephan Fuhrmann (stephan.fuhrmann AT 1und1.de)
 *
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by Stanford University.  The name of the University may not be used
 * to endorse or promote products derived from this software without
 * specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "config.h"
#include "fping.h"
#include "flags.h"

#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <net/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include <netinet/icmp6.h>

char* ping_buffer_ipv6 = 0;
size_t ping_pkt_size_ipv6;

/* Interface index for outgoing packets (0 = not set, use routing table) */
static int outgoing_iface_idx_ipv6 = 0;

int open_ping_socket_ipv6(int *socktype)
{
    struct protoent* proto;
    int s;

    /* confirm that ICMP6 is available on this machine */
    if ((proto = getprotobyname("ipv6-icmp")) == NULL)
        crash_and_burn("ipv6-icmp: unknown protocol");

    /* create raw socket for ICMP6 calls (ping) */
    *socktype = SOCK_RAW;
    s = socket(AF_INET6, *socktype, proto->p_proto);
    if (s < 0) {
        /* try non-privileged icmp6 (works on Mac OSX without privileges, for example) */
        *socktype = SOCK_DGRAM;
        s = socket(AF_INET6, *socktype, proto->p_proto);
        if (s < 0) {
            return -1;
        }
    } else {
        /* receive only ICMP6 messages relevant for fping on raw socket */
        struct icmp6_filter recv_filter;

        ICMP6_FILTER_SETBLOCKALL(&recv_filter);
        ICMP6_FILTER_SETPASS(ICMP6_ECHO_REPLY, &recv_filter);
        ICMP6_FILTER_SETPASS(ICMP6_DST_UNREACH, &recv_filter);
        ICMP6_FILTER_SETPASS(ICMP6_PACKET_TOO_BIG, &recv_filter);
        ICMP6_FILTER_SETPASS(ICMP6_TIME_EXCEEDED, &recv_filter);
        ICMP6_FILTER_SETPASS(ICMP6_PARAM_PROB, &recv_filter);

        if (setsockopt(s, IPPROTO_ICMPV6, ICMP6_FILTER, &recv_filter, sizeof(recv_filter))) {
            errno_crash_and_burn("cannot set icmp6 message type filter");
        }
    }

    /* Make sure that we use non-blocking IO */
    {
        int flags;

        if ((flags = fcntl(s, F_GETFL, 0)) < 0)
            perror("fcntl");

        if (fcntl(s, F_SETFL, flags | O_NONBLOCK) < 0)
            perror("fcntl");
    }

    return s;
}

void socket_set_outgoing_iface_ipv6(int s, const char *iface_name)
{
    unsigned int idx = if_nametoindex(iface_name);
    if (idx == 0) {
        fprintf(stderr, "fping: unknown interface '%s'\n", iface_name);
        exit(1);
    }
    outgoing_iface_idx_ipv6 = (int)idx;

    int on = 1;
    if (setsockopt(s, IPPROTO_IPV6, IPV6_RECVPKTINFO, &on, sizeof(on)) < 0) {
        perror("setsockopt IPV6_RECVPKTINFO");
        exit(1);
    }
}

void init_ping_buffer_ipv6(size_t ping_data_size)
{
    /* allocate ping buffer */
    ping_pkt_size_ipv6 = ping_data_size + sizeof(struct icmp6_hdr);
    ping_buffer_ipv6 = (char*)calloc(1, ping_pkt_size_ipv6);
    if (!ping_buffer_ipv6)
        crash_and_burn("can't malloc ping packet");
}

void socket_set_src_addr_ipv6(int s, struct in6_addr* src_addr, int *ident)
{
    struct sockaddr_in6 sa;
    socklen_t len = sizeof(sa);

    memset(&sa, 0, sizeof(sa));
    sa.sin6_family = AF_INET6;
    sa.sin6_addr = *src_addr;
    if (bind(s, (struct sockaddr*)&sa, sizeof(sa)) < 0)
        errno_crash_and_burn("cannot bind source address");

    if (ident) {
        memset(&sa, 0, len);
        if (getsockname(s, (struct sockaddr *)&sa, &len) < 0)
            errno_crash_and_burn("can't get ICMP6 socket identity");

        if (sa.sin6_port)
            *ident = sa.sin6_port;
    }
}

int socket_sendto_ping_ipv6(int s, struct sockaddr* saddr, socklen_t saddr_len, uint16_t icmp_seq_nr, uint16_t icmp_id_nr)
{
    struct icmp6_hdr* icp;
    int n;

    icp = (struct icmp6_hdr*)ping_buffer_ipv6;
    icp->icmp6_type = ICMP6_ECHO_REQUEST;
    icp->icmp6_code = 0;
    icp->icmp6_seq = htons(icmp_seq_nr);
    icp->icmp6_id = icmp_id_nr;

    if (opt_random_data_on) {
        for (n = sizeof(struct icmp6_hdr); n < ping_pkt_size_ipv6; ++n) {
            ping_buffer_ipv6[n] = random() & 0xFF;
        }
    }

    icp->icmp6_cksum = 0; /* The IPv6 stack calculates the checksum for us... */

    if (outgoing_iface_idx_ipv6 > 0) {
        struct iovec iov = {
            .iov_base = icp,
            .iov_len  = ping_pkt_size_ipv6
        };

        char cmsg_buf[CMSG_SPACE(sizeof(struct in6_pktinfo))];
        memset(cmsg_buf, 0, sizeof(cmsg_buf));

        struct msghdr msg = {
            .msg_name       = saddr,
            .msg_namelen    = saddr_len,
            .msg_iov        = &iov,
            .msg_iovlen     = 1,
            .msg_control    = cmsg_buf,
            .msg_controllen = sizeof(cmsg_buf),
            .msg_flags      = 0
        };

        struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
        cmsg->cmsg_level = IPPROTO_IPV6;
        cmsg->cmsg_type  = IPV6_PKTINFO;
        cmsg->cmsg_len   = CMSG_LEN(sizeof(struct in6_pktinfo));

        struct in6_pktinfo *pktinfo = (struct in6_pktinfo *)CMSG_DATA(cmsg);
        memset(pktinfo, 0, sizeof(*pktinfo));
        pktinfo->ipi6_ifindex = outgoing_iface_idx_ipv6;

        n = sendmsg(s, &msg, 0);
    } else {
        n = sendto(s, icp, ping_pkt_size_ipv6, 0, saddr, saddr_len);
    }

    return n;
}
