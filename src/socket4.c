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
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <net/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>

char* ping_buffer_ipv4 = 0;
size_t ping_pkt_size_ipv4;

/* Interface index for outgoing packets (0 = not set, use routing table) */
static int outgoing_iface_idx_ipv4 = 0;

/* Source address for outgoing packets (used to populate ipi_spec_dst) */
static struct in_addr outgoing_src_addr_ipv4;
static int outgoing_src_addr_set_ipv4 = 0;

int open_ping_socket_ipv4(int *socktype)
{
    struct protoent* proto;
    int s;

    /* confirm that ICMP is available on this machine */
    if ((proto = getprotobyname("icmp")) == NULL)
        crash_and_burn("icmp: unknown protocol");

    /* create raw socket for ICMP calls (ping) */
    *socktype = SOCK_RAW;
    s = socket(AF_INET, *socktype, proto->p_proto);
    if (s < 0) {
        /* try non-privileged icmp (works on Mac OSX without privileges, for example) */
        *socktype = SOCK_DGRAM;
        s = socket(AF_INET, *socktype, proto->p_proto);
        if (s < 0) {
            return -1;
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

void socket_set_outgoing_iface_ipv4(int s, const char *iface_name)
{
    unsigned int idx = if_nametoindex(iface_name);
    if (idx == 0) {
        fprintf(stderr, "fping: unknown interface '%s'\n", iface_name);
        exit(1);
    }
    outgoing_iface_idx_ipv4 = (int)idx;

    int on = 1;
    if (setsockopt(s, IPPROTO_IP, IP_PKTINFO, &on, sizeof(on)) < 0) {
        perror("setsockopt IP_PKTINFO");
        exit(1);
    }
}

void init_ping_buffer_ipv4(size_t ping_data_size)
{
    /* allocate ping buffer */
    ping_pkt_size_ipv4 = ping_data_size + ICMP_MINLEN;
    ping_buffer_ipv4 = (char*)calloc(1, ping_pkt_size_ipv4);
    if (!ping_buffer_ipv4)
        crash_and_burn("can't malloc ping packet");
}

void socket_set_src_addr_ipv4(int s, struct in_addr* src_addr, int *ident)
{
    struct sockaddr_in sa;
    socklen_t len = sizeof(sa);

    /* Remember source address so we can set ipi_spec_dst in sendmsg() */
    outgoing_src_addr_ipv4 = *src_addr;
    outgoing_src_addr_set_ipv4 = 1;

    memset(&sa, 0, len);
    sa.sin_family = AF_INET;
    sa.sin_addr = *src_addr;
    if (bind(s, (struct sockaddr*)&sa, len) < 0)
        errno_crash_and_burn("cannot bind source address");

    if (ident) {
        memset(&sa, 0, len);
        if (getsockname(s, (struct sockaddr *)&sa, &len) < 0)
            errno_crash_and_burn("can't get ICMP socket identity");

        if (sa.sin_port)
            *ident = sa.sin_port;
    }
}

unsigned short calcsum(unsigned short* buffer, int length)
{
    unsigned long sum;

    /* initialize sum to zero and loop until length (in words) is 0 */
    for (sum = 0; length > 1; length -= 2) /* sizeof() returns number of bytes, we're interested in number of words */
        sum += *buffer++; /* add 1 word of buffer to sum and proceed to the next */

    /* we may have an extra byte */
    if (length == 1)
        sum += (char)*buffer;

    sum = (sum >> 16) + (sum & 0xFFFF); /* add high 16 to low 16 */
    sum += (sum >> 16); /* add carry */
    return ~sum;
}

int socket_sendto_ping_ipv4(int s, struct sockaddr* saddr, socklen_t saddr_len, uint16_t icmp_seq_nr, uint16_t icmp_id_nr, uint8_t icmp_proto, int ttl)
{
    struct icmp* icp;
    struct timespec tsorig;
    long tsorig_ms;
    int n;

    icp = (struct icmp*)ping_buffer_ipv4;

    icp->icmp_type = icmp_proto;
    if(icmp_proto == ICMP_TSTAMP) {
        clock_gettime(CLOCK_REALTIME, &tsorig);
        tsorig_ms = (tsorig.tv_sec % (24*60*60)) * 1000 + tsorig.tv_nsec / 1000000;
        icp->icmp_otime = htonl(tsorig_ms);
        icp->icmp_rtime = 0;
        icp->icmp_ttime = 0;
    }
    icp->icmp_code = 0;
    icp->icmp_cksum = 0;
    icp->icmp_seq = htons(icmp_seq_nr);
    icp->icmp_id = icmp_id_nr;

    if (opt_random_data_on) {
        for (n = ((char*)&icp->icmp_data - (char*)icp); n < ping_pkt_size_ipv4; ++n) {
            ping_buffer_ipv4[n] = random() & 0xFF;
        }
    }

    icp->icmp_cksum = calcsum((unsigned short*)icp, ping_pkt_size_ipv4);


    struct iovec iov = {
        .iov_base = icp,
        .iov_len  = ping_pkt_size_ipv4
    };

    /* Buffer for ancillary data (Interface info and/or TTL) */
    char cmsg_buf[CMSG_SPACE(sizeof(struct in_pktinfo))];
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

    if (outgoing_iface_idx_ipv4 > 0) {
        msg.msg_controllen += CMSG_SPACE(sizeof(struct in_pktinfo));
        cmsg->cmsg_level = IPPROTO_IP;
        cmsg->cmsg_type  = IP_PKTINFO;
        cmsg->cmsg_len   = CMSG_LEN(sizeof(struct in_pktinfo));

        struct in_pktinfo *pktinfo = (struct in_pktinfo *)CMSG_DATA(cmsg);
        memset(pktinfo, 0, sizeof(*pktinfo));
        pktinfo->ipi_ifindex = outgoing_iface_idx_ipv4;
        if (outgoing_src_addr_set_ipv4) {
            pktinfo->ipi_spec_dst = outgoing_src_addr_ipv4;
        }
        cmsg = CMSG_NXTHDR(&msg, cmsg);
    }

    /* Handle TTL */
    if (ttl > 0) {
        msg.msg_controllen += CMSG_SPACE(sizeof(int));
        cmsg->cmsg_level = IPPROTO_IP;
        cmsg->cmsg_type  = IP_TTL;
        cmsg->cmsg_len   = CMSG_LEN(sizeof(int));
        memcpy(CMSG_DATA(cmsg), &ttl, sizeof(int));
    }

    /* Use sendmsg if we have ancillary data, otherwise fallback to sendto */
    if (msg.msg_controllen > 0) {
        n = sendmsg(s, &msg, 0);
    } else {
        n = sendto(s, icp, ping_pkt_size_ipv4, 0, saddr, saddr_len);
    }

    return n;
}
