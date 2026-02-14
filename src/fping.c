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

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "config.h"
#include "fping.h"
#include "flags.h"
#include "options.h"
#include "optparse.h"

#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "seqmap.h"
#include "output.h"
#include "stats.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#include <stddef.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>

#if HAVE_SYS_FILE_H
#include <sys/file.h>
#endif /* HAVE_SYS_FILE_H */

#ifdef IPV6
#include <netinet/icmp6.h>
#include <netinet/ip6.h>
#endif
#include <netinet/in_systm.h>

#include <netinet/ip.h>
#include <netinet/ip_icmp.h>

#include <arpa/inet.h>
#include <ctype.h>
#include <netdb.h>

#include <sys/select.h>

/*** compatibility ***/

/* Mac OS X's getaddrinfo() does not fail if we use an invalid combination,
 * e.g. AF_INET6 with "127.0.0.1". If we pass AI_UNUSABLE to flags, it behaves
 * like other platforms. But AI_UNUSABLE isn't available on other platforms,
 * and we can safely use 0 for flags instead.
 */
#ifndef AI_UNUSABLE
#define AI_UNUSABLE 0
#endif

/* MSG_TRUNC available on Linux kernel 2.2+, makes recvmsg return the full
 * length of the raw packet received, even if the buffer is smaller */
#ifndef MSG_TRUNC
#define MSG_TRUNC 0
#define RECV_BUFSIZE 4096
#else
#define RECV_BUFSIZE 128
#endif

/*** externals ***/

extern char *optarg;
extern int optind, opterr;
#ifndef h_errno
extern int h_errno;
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

/*** Constants ***/

/* CLOCK_MONTONIC starts under macOS, OpenBSD and FreeBSD with undefined positive point and can not be use
 * see github PR #217
 * The configure script detect the predefined operating systems an set CLOCK_REALTIME using over ONLY_CLOCK_REALTIME variable
 */
#if HAVE_SO_TIMESTAMPNS || ONLY_CLOCK_REALTIME
#define CLOCKID CLOCK_REALTIME
#endif

#if !defined(CLOCKID)
#if defined(CLOCK_MONOTONIC)
#define CLOCKID CLOCK_MONOTONIC
#else
#define CLOCKID CLOCK_REALTIME
#endif
#endif

/*** Ping packet defines ***/

#define MAX_IP_PACKET 65535 /* (theoretical) max IPv4 packet size */
#define SIZE_IP_HDR 20 /* min IPv4 header size */
#define SIZE_ICMP_HDR 8 /* from ip_icmp.h */
#define MAX_PING_DATA (MAX_IP_PACKET - SIZE_IP_HDR - SIZE_ICMP_HDR)

#define MAX_GENERATE 131072 /* maximum number of hosts that -g can generate */
#define MAX_TARGET_NAME_LEN 255 /* maximum target name length read from file */

/* ICMP Timestamp has a fixed payload size of 12 bytes */
#define ICMP_TIMESTAMP_DATA_SIZE 12

#ifdef FPING_SAFE_LIMITS
#define MIN_INTERVAL_MS 1 /* in millisec */
#define MIN_PERHOST_INTERVAL_MS 10 /* in millisec */
#else
#define MIN_INTERVAL_MS 0
/* Set a very low limit for the per-host interval, even if safe limits are
 * disabled, so that the memory allocation of the event storage is not
 * unreasonably high. 0.001 ms would mean in theory at least 592 mbps of data
 * sent to a single host, which probably doesn't make sense in any scenario. */
#define MIN_PERHOST_INTERVAL_MS 0.001
#endif

/* response time array flags */
#define RESP_WAITING -1
#define RESP_UNUSED -2
#define RESP_ERROR -3
#define RESP_TIMEOUT -4

/* debugging flags */
#if defined(DEBUG) || defined(_DEBUG)
#define DBG_TRACE 1
#define DBG_SENT_TIMES 2
#define DBG_RANDOM_LOSE_FEW 4
#define DBG_RANDOM_LOSE_MANY 8
#define DBG_PRINT_PER_SYSTEM 16
#define DBG_REPORT_ALL_RTTS 32
#endif /* DEBUG || _DEBUG */

/* Long names for ICMP packet types */
#define ICMP_TYPE_STR_MAX 18
char *icmp_type_str[19] = {
    "ICMP Echo Reply", /* 0 */
    "",
    "",
    "ICMP Unreachable", /* 3 */
    "ICMP Source Quench", /* 4 */
    "ICMP Redirect", /* 5 */
    "",
    "",
    "ICMP Echo", /* 8 */
    "",
    "",
    "ICMP Time Exceeded", /* 11 */
    "ICMP Parameter Problem", /* 12 */
    "ICMP Timestamp Request", /* 13 */
    "ICMP Timestamp Reply", /* 14 */
    "ICMP Information Request", /* 15 */
    "ICMP Information Reply", /* 16 */
    "ICMP Mask Request", /* 17 */
    "ICMP Mask Reply" /* 18 */
};

char *icmp_unreach_str[16] = {
    "ICMP Network Unreachable", /* 0 */
    "ICMP Host Unreachable", /* 1 */
    "ICMP Protocol Unreachable", /* 2 */
    "ICMP Port Unreachable", /* 3 */
    "ICMP Unreachable (Fragmentation Needed)", /* 4 */
    "ICMP Unreachable (Source Route Failed)", /* 5 */
    "ICMP Unreachable (Destination Network Unknown)", /* 6 */
    "ICMP Unreachable (Destination Host Unknown)", /* 7 */
    "ICMP Unreachable (Source Host Isolated)", /* 8 */
    "ICMP Unreachable (Communication with Network Prohibited)", /* 9 */
    "ICMP Unreachable (Communication with Host Prohibited)", /* 10 */
    "ICMP Unreachable (Network Unreachable For Type Of Service)", /* 11 */
    "ICMP Unreachable (Host Unreachable For Type Of Service)", /* 12 */
    "ICMP Unreachable (Communication Administratively Prohibited)", /* 13 */
    "ICMP Unreachable (Host Precedence Violation)", /* 14 */
    "ICMP Unreachable (Precedence cutoff in effect)" /* 15 */
};

#define ICMP_UNREACH_MAXTYPE 15

#ifdef IPV6
/* Long names for ICMPv6 unreachable codes */
#define ICMP6_UNREACH_MAXCODE 9
char *icmp6_unreach_str[ICMP6_UNREACH_MAXCODE + 1] = {
    "No route to destination", /* 0 */
    "Communication with destination administratively prohibited", /* 1 */
    "Beyond scope of source address", /* 2 */
    "Address unreachable", /* 3 */
    "Port unreachable", /* 4 */
    "Source address failed ingress/egress policy", /* 5 */
    "Reject route to destination", /* 6 */
    "Error in Source Routing Header", /* 7 */
    "Headers too long", /* 8 */
    "Error in P-Route", /* 9 */
};

/* Long names for ICMPv6 time exceeded codes */
#define ICMP6_TIME_EXCEEDED_MAXCODE 1
char *icmp6_time_exceeded_str[ICMP6_TIME_EXCEEDED_MAXCODE + 1] = {
    "Hop limit exceeded in transit", /* 0 */
    "Fragment reassembly time exceeded", /* 1 */
};

/* Long names for ICMPv6 parameter problem codes */
#define ICMP6_PARAM_PROB_MAXCODE 10
char *icmp6_param_prob_str[ICMP6_PARAM_PROB_MAXCODE + 1] = {
    "Erroneous header field encountered", /* 0 */
    "Unrecognized Next Header type encountered", /* 1 */
    "Unrecognized IPv6 option encountered", /* 2 */
    "IPv6 First Fragment has incomplete IPv6 Header Chain", /* 3 */
    "SR Upper-layer Header Error", /* 4 */
    "Unrecognized Next Header type encountered by intermediate node", /* 5 */
    "Extension header too big", /* 6 */
    "Extension header chain too long", /* 7 */
    "Too many extension headers", /* 8 */
    "Too many options in extension header", /* 9 */
    "Option too big", /* 10 */
};
#endif

IP_HEADER_RESULT default_ip_header_result() {
    IP_HEADER_RESULT res;
    res.tos = -1;
    res.ttl = -1;
    res.otime_ms = 0x80000000U;
    res.rtime_ms = 0x80000000U;
    res.ttime_ms = 0x80000000U;
    res.src_addr[0] = '\0';
    return res;
}

int event_storage_count;

/*** globals ***/

HOST_ENTRY **table = NULL; /* array of pointers to items in the list */

/* we keep two separate queues: a ping queue, for when the next ping should be
 * sent, and a timeout queue. the reason for having two separate queues is that
 * the ping period and the timeout value are different, so if we put them in
 * the same event queue, we would need to scan many more entries when inserting
 * into the sorted list.
 */
struct event_queue event_queue_ping;
struct event_queue event_queue_timeout;

char *prog;
int ident4 = 0; /* our icmp identity field */
int ident6 = 0;
const int sock_opt_on = 1; /* to activate a socket option */
int socket4 = -1;
int socktype4 = -1;
int using_sock_dgram4 = 0;
#ifndef IPV6
int hints_ai_family = AF_INET;
#else
int socket6 = -1;
int socktype6 = -1;
int hints_ai_family = AF_UNSPEC;
#endif

volatile sig_atomic_t status_snapshot = 0;
volatile sig_atomic_t finish_requested = 0;

unsigned int debugging = 0;

unsigned int trials;
int64_t report_interval = 0;
int src_addr_set = 0;
struct in_addr src_addr;
#ifdef IPV6
int src_addr6_set = 0;
struct in6_addr src_addr6;
#endif

/* global stats */
int64_t max_reply = 0;
int64_t min_reply = 0;
int64_t total_replies = 0;
int64_t sum_replies = 0;
int max_hostname_len = 0;
int num_hosts = 0; /* total number of hosts */
int num_alive = 0, /* total number alive */
    num_unreachable = 0, /* total number unreachable */
    num_noaddress = 0; /* total number of addresses not found */
int num_timeout = 0, /* number of times select timed out */
    num_pingsent = 0, /* total pings sent */
    num_pingreceived = 0, /* total pings received */
    num_othericmprcvd = 0; /* total non-echo-reply ICMP received */

struct timespec current_time; /* current time (pseudo) */
int64_t current_time_ns;
int64_t start_time;
int64_t end_time;
int64_t last_send_time; /* time last ping was sent */
int64_t next_report_time; /* time next -Q report is expected */


#if defined(DEBUG) || defined(_DEBUG)
int opt_debug_randomly_lose_on, opt_debug_trace_on, opt_debug_print_per_system_on;
int lose_factor;
#endif /* DEBUG || _DEBUG */

unsigned int fwmark = 0;

char *filename = NULL; /* file containing hosts to ping */

/************************************************************

  Function: p_setsockopt

*************************************************************

  Inputs:  p_uid: privileged uid. Others as per setsockopt(2)

  Description:

  Elevates privileges to p_uid when required, calls
  setsockopt, and drops privileges back.

************************************************************/

int p_setsockopt(uid_t p_uid, int sockfd, int level, int optname,
    const void *optval, socklen_t optlen)
{
    const uid_t saved_uid = geteuid();
    int res;

    if (p_uid != saved_uid && seteuid(p_uid)) {
        perror("cannot elevate privileges for setsockopt");
    }

    res = setsockopt(sockfd, level, optname, optval, optlen);

    if (p_uid != saved_uid && seteuid(saved_uid)) {
        perror("fatal error: could not drop privileges after setsockopt");
        /* continuing would be a security hole */
        exit(4);
    }

    return res;
}

unsigned long strtoul_strict(const char *arg, int base)
{
    char *endptr;
    unsigned long val;

    while (isspace(*arg))
        arg++;

    if (arg[0] == '-')
        usage(1);

    errno = 0;
    val = strtoul(arg, &endptr, base);
    if (errno != 0 || arg == endptr || *endptr != '\0')
        usage(1);

    return val;
}

double strtod_strict(const char *arg)
{
    char *endptr;
    double val;

    errno = 0;
    val = strtod(arg, &endptr);
    if (errno != 0 || arg == endptr || *endptr != '\0' || val < 0)
        usage(1);

    return val;
}

/************************************************************

  Function: main

*************************************************************

  Inputs:  int argc, char** argv

  Description:

  Main program entry point

************************************************************/

int main(int argc, char **argv)
{
/* Debug: CPU Performance */
#if defined(DEBUG) || defined(_DEBUG)
    clock_t perf_cpu_start, perf_cpu_end;
    double perf_cpu_time_used;
    perf_cpu_start = clock();
#endif /* DEBUG || _DEBUG */

    int c;
    char *endptr;
    const uid_t suid = geteuid();
    int tos = 0;
    struct optparse optparse_state;
#ifdef USE_SIGACTION
    struct sigaction act;
#endif

    /* pre-parse -h/--help, so that we also can output help information
     * without trying to open the socket, which might fail */
    prog = argv[0];
    if (argc == 2 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)) {
        usage(0);
    }

    socket4 = open_ping_socket_ipv4(&socktype4);
#ifdef __linux__
    /* We only treat SOCK_DGRAM differently on Linux, where the IPv4 header
     * structure is missing in the message.
     */
    using_sock_dgram4 = (socktype4 == SOCK_DGRAM);
#endif

#ifdef IPV6
    socket6 = open_ping_socket_ipv6(&socktype6);
    /* if called (sym-linked) via 'fping6', imply '-6'
     * for backward compatibility */
    if (strstr(prog, "fping6")) {
        hints_ai_family = AF_INET6;
    }
#endif

    memset(&src_addr, 0, sizeof(src_addr));
#ifdef IPV6
    memset(&src_addr6, 0, sizeof(src_addr6));
#endif

    if (!suid && suid != getuid()) {
        /* *temporarily* drop privileges */
        if (seteuid(getuid()) == -1)
            perror("cannot setuid");
    }

    optparse_init(&optparse_state, argv);
    ident4 = ident6 = htons(getpid() & 0xFFFF);
    opt_verbose_on = 1;
    opt_backoff_on = 1;
    opterr = 1;

    /* get command line options */

    struct optparse_long longopts[] = {
        { "ipv4", '4', OPTPARSE_NONE },
        { "ipv6", '6', OPTPARSE_NONE },
        { "alive", 'a', OPTPARSE_NONE },
        { "addr", 'A', OPTPARSE_NONE },
        { "size", 'b', OPTPARSE_REQUIRED },
        { "backoff", 'B', OPTPARSE_REQUIRED },
        { "count", 'c', OPTPARSE_REQUIRED },
        { "vcount", 'C', OPTPARSE_REQUIRED },
        { "rdns", 'd', OPTPARSE_NONE },
        { "timestamp", 'D', OPTPARSE_NONE },
        { "timestamp-format", 0, OPTPARSE_REQUIRED },
        { "elapsed", 'e', OPTPARSE_NONE },
        { "file", 'f', OPTPARSE_REQUIRED },
        { "generate", 'g', OPTPARSE_NONE },
        { "help", 'h', OPTPARSE_NONE },
        { "ttl", 'H', OPTPARSE_REQUIRED },
        { "interval", 'i', OPTPARSE_REQUIRED },
        { "iface", 'I', OPTPARSE_REQUIRED },
        { "oiface", 0, OPTPARSE_REQUIRED },
        { "json", 'J', OPTPARSE_NONE },
        { "icmp-timestamp", 0, OPTPARSE_NONE },
#ifdef SO_MARK
        { "fwmark", 'k', OPTPARSE_REQUIRED },
#endif
        { "loop", 'l', OPTPARSE_NONE },
        { "all", 'm', OPTPARSE_NONE },
        { "dontfrag", 'M', OPTPARSE_NONE },
        { "name", 'n', OPTPARSE_NONE },
        { "netdata", 'N', OPTPARSE_NONE },
        { "outage", 'o', OPTPARSE_NONE },
        { "tos", 'O', OPTPARSE_REQUIRED },
        { "period", 'p', OPTPARSE_REQUIRED },
        { "quiet", 'q', OPTPARSE_NONE },
        { "squiet", 'Q', OPTPARSE_REQUIRED },
        { "retry", 'r', OPTPARSE_REQUIRED },
        { "random", 'R', OPTPARSE_NONE },
        { "stats", 's', OPTPARSE_NONE },
        { "src", 'S', OPTPARSE_REQUIRED },
        { "timeout", 't', OPTPARSE_REQUIRED },
        { NULL, 'T', OPTPARSE_REQUIRED },
        { "unreach", 'u', OPTPARSE_NONE },
        { "version", 'v', OPTPARSE_NONE },
        { "reachable", 'x', OPTPARSE_REQUIRED },
        { "fast-reachable", 'X', OPTPARSE_REQUIRED },
        { "check-source", 0, OPTPARSE_NONE },
        { "print-tos", 0, OPTPARSE_NONE },
        { "print-ttl", 0, OPTPARSE_NONE },
        { "print-srcaddr", 0, OPTPARSE_NONE },
        { "seqmap-timeout", 0, OPTPARSE_REQUIRED },
#if defined(DEBUG) || defined(_DEBUG)
        { NULL, 'z', OPTPARSE_REQUIRED },
#endif
        { 0, 0, 0 }
    };

    double opt_val_double;
    while ((c = optparse_long(&optparse_state, longopts, NULL)) != EOF) {
        switch (c) {
        case 0:
            if(strstr(optparse_state.optlongname, "timestamp-format") != NULL) {
                if(strcmp(optparse_state.optarg, "ctime") == 0) {
                  opt_timestamp_format = 1;
                }else if(strcmp(optparse_state.optarg, "iso") == 0) {
                  opt_timestamp_format = 2;
                }else if(strcmp(optparse_state.optarg, "rfc3339") == 0) {
                  opt_timestamp_format = 3;
                }else{
                  usage(1);
                }
            } else if (strstr(optparse_state.optlongname, "check-source") != NULL) {
                opt_check_source_on = 1;
            } else if (strstr(optparse_state.optlongname, "icmp-timestamp") != NULL) {
#ifdef IPV6
                if (hints_ai_family != AF_UNSPEC && hints_ai_family != AF_INET) {
                    fprintf(stderr, "%s: ICMP Timestamp is IPv4 only\n", prog);
                    exit(1);
                }
                hints_ai_family = AF_INET;
#endif
                opt_icmp_request_typ = 13;
                opt_ping_data_size = ICMP_TIMESTAMP_DATA_SIZE;
            } else if (strstr(optparse_state.optlongname, "print-tos") != NULL) {
                opt_print_tos_on = 1;
#if defined(HAVE_IP_RECVTOS)
                if (socket4 >= 0 && (socktype4 == SOCK_DGRAM)) {
                    if (setsockopt(socket4, IPPROTO_IP, IP_RECVTOS, &sock_opt_on, sizeof(sock_opt_on))) {
                        perror("setsockopt IP_RECVTOS");
                    }
                }
#endif
#if defined(IPV6) && defined(IPV6_RECVTCLASS)
                if (socket6 >= 0) {
                    if (setsockopt(socket6, IPPROTO_IPV6, IPV6_RECVTCLASS, &sock_opt_on, sizeof(sock_opt_on))) {
                        perror("setsockopt IPV6_RECVTCLASS");
                    }
                }
#endif
            } else if (strstr(optparse_state.optlongname, "print-ttl") != NULL) {
                opt_print_ttl_on = 1;
                if (socket4 >= 0 && (socktype4 == SOCK_DGRAM)) {
                    if (setsockopt(socket4, IPPROTO_IP, IP_RECVTTL, &sock_opt_on, sizeof(sock_opt_on))) {
                        perror("setsockopt IP_RECVTTL");
                    }
                }
#if defined(IPV6) && defined(IPV6_RECVHOPLIMIT)
                if (socket6 >= 0) {
                    if (setsockopt(socket6, IPPROTO_IPV6, IPV6_RECVHOPLIMIT, &sock_opt_on, sizeof(sock_opt_on))) {
                        perror("setsockopt IPV6_RECVHOPLIMIT");
                    }
                }
#endif
            } else if (strstr(optparse_state.optlongname, "print-srcaddr") != NULL) {
                opt_print_srcaddr_on = 1;
            } else if (strstr(optparse_state.optlongname, "seqmap-timeout") != NULL) {
                opt_seqmap_timeout = strtod_strict(optparse_state.optarg) * 1000000;
            } else if (strstr(optparse_state.optlongname, "oiface") != NULL) {
                opt_oiface_on = 1;
#ifdef IP_PKTINFO
                if (socket4 >= 0) {
                    socket_set_outgoing_iface_ipv4(socket4, optparse_state.optarg);
                }
#ifdef IPV6
                if (socket6 >= 0) {
                    socket_set_outgoing_iface_ipv6(optparse_state.optarg);
                }
#endif
#else
                fprintf(stderr, "%s: --oiface is not supported on this platform (IP_PKTINFO unavailable)\n", prog);
                exit(3);
#endif
            } else {
                usage(1);
            }
            break;
        case '4':
#ifdef IPV6
            if (hints_ai_family != AF_UNSPEC && hints_ai_family != AF_INET) {
                fprintf(stderr, "%s: can't specify both -4 and -6\n", prog);
                exit(1);
            }
            hints_ai_family = AF_INET;
#endif
            break;
        case '6':
#ifdef IPV6
            if (hints_ai_family != AF_UNSPEC && hints_ai_family != AF_INET6) {
                fprintf(stderr, "%s: can't specify both -4 and -6\n", prog);
                exit(1);
            }
            hints_ai_family = AF_INET6;
#else
            fprintf(stderr, "%s: IPv6 not supported by this binary\n", prog);
            exit(1);
#endif
            break;
        case 'M':
#ifdef IP_MTU_DISCOVER
            if (socket4 >= 0) {
                int val = IP_PMTUDISC_DO;
                if (setsockopt(socket4, IPPROTO_IP, IP_MTU_DISCOVER, &val, sizeof(val))) {
                    perror("setsockopt IP_MTU_DISCOVER");
                }
            }
#ifdef IPV6
            if (socket6 >= 0) {
                int val = IPV6_PMTUDISC_DO;
                if (setsockopt(socket6, IPPROTO_IPV6, IPV6_MTU_DISCOVER, &val, sizeof(val))) {
                    perror("setsockopt IPV6_MTU_DISCOVER");
                }
            }
#endif
#else
            fprintf(stderr, "%s, -M option not supported on this platform\n", prog);
            exit(1);
#endif
            break;

        case 't':
            opt_timeout = strtod_strict(optparse_state.optarg) * 1000000;
            opt_timeout_on = 1;
            break;

        case 'r':
            opt_retry = (unsigned int)strtoul_strict(optparse_state.optarg, 10);
            break;

        case 'i':
            opt_interval = strtod_strict(optparse_state.optarg) * 1000000;
            break;

        case 'p':
            opt_perhost_interval = strtod_strict(optparse_state.optarg) * 1000000;
            break;

        case 'c':
            opt_count = (unsigned int)strtoul_strict(optparse_state.optarg, 10);
            if (!opt_count)
                usage(1);

            opt_count_on = 1;
            break;

        case 'C':
            opt_count = (unsigned int)strtoul_strict(optparse_state.optarg, 10);
            if (!opt_count)
                usage(1);

            opt_count_on = 1;
            opt_report_all_rtts_on = 1;
            break;

        case 'b':
            opt_ping_data_size = (unsigned int)strtoul_strict(optparse_state.optarg, 10);
            opt_size_on = 1;
            break;

        case 'h':
            usage(0);
            break;

        case 'q':
            opt_verbose_on = 0;
            opt_quiet_on = 1;
            break;

        case 'Q':
            opt_verbose_on = 0;
            opt_quiet_on = 1;
            errno = 0;
            opt_val_double = strtod(optparse_state.optarg, &endptr);
            if (errno != 0 || optparse_state.optarg == endptr || (*endptr != '\0' && *endptr != ','))
                usage(1);
            if (opt_val_double < 0) {
                usage(1);
            }
            report_interval = opt_val_double * 1e9;

            /* recognize keyword(s) after number, ignore everything else */
            {
                char *comma = strchr(optparse_state.optarg, ',');
                if ((comma != NULL) && (strcmp(++comma, "cumulative") == 0)) {
                    opt_cumulative_stats_on = 1;
                }
            }

            break;

        case 'e':
            opt_elapsed_on = 1;
            break;

        case 'm':
            opt_multif_on = 1;
            break;

        case 'N':
            opt_print_netdata_on = 1;
            break;

        case 'n':
            opt_name_on = 1;
            if (opt_rdns_on) {
                fprintf(stderr, "%s: use either one of -d or -n\n", prog);
                exit(1);
            }
            break;

        case 'd':
            opt_rdns_on = 1;
            if (opt_name_on) {
                fprintf(stderr, "%s: use either one of -d or -n\n", prog);
                exit(1);
            }
            break;

        case 'A':
            opt_addr_on = 1;
            break;

        case 'B':
            opt_backoff = strtod_strict(optparse_state.optarg);
            break;

        case 's':
            opt_stats_on = 1;
            break;

        case 'D':
            opt_timestamp_on = 1;
            break;

        case 'R':
            opt_random_data_on = 1;
            break;

        case 'l':
            opt_loop_on = 1;
            opt_backoff_on = 0;
            break;

        case 'u':
            opt_unreachable_on = 1;
            break;

        case 'a':
            opt_alive_on = 1;
            break;

        case 'H':
            opt_ttl = (unsigned int)strtoul_strict(optparse_state.optarg, 10);
            if (!opt_ttl)
                usage(1);
            break;

#if defined(DEBUG) || defined(_DEBUG)
        case 'z':
            debugging = (unsigned int)strtoul_strict(optparse_state.optarg, 0);
            break;
#endif /* DEBUG || _DEBUG */

        case 'v':
            printf("%s: Version %s\n", prog, VERSION);
            exit(0);

        case 'x':
            opt_min_reachable = (unsigned int)strtoul_strict(optparse_state.optarg, 10);
            if (!opt_min_reachable)
                usage(1);
            break;

        case 'X':
            opt_min_reachable = (unsigned int)strtoul_strict(optparse_state.optarg, 10);
            if (!opt_min_reachable)
                usage(1);
            opt_fast_reachable_on = 1;
            break;

        case 'f':
            filename = optparse_state.optarg;
            break;
#ifdef SO_MARK
        case 'k':
            fwmark = (unsigned int)strtoul_strict(optparse_state.optarg, 10);
            if (!fwmark)
                usage(1);

            if (socket4 >= 0)
                if(-1 == p_setsockopt(suid, socket4, SOL_SOCKET, SO_MARK, &fwmark, sizeof fwmark))
                    perror("fwmark ipv4");

#ifdef IPV6
            if (socket6 >= 0)
                if(-1 == p_setsockopt(suid, socket6, SOL_SOCKET, SO_MARK, &fwmark, sizeof fwmark))
                    perror("fwmark ipv6");
#endif

            break;
#endif

        case 'g':
            /* use IP list generation */
            /* mutually exclusive with using file input or command line targets */
            opt_generate_on = 1;
            break;

        case 'S':
            if (inet_pton(AF_INET, optparse_state.optarg, &src_addr)) {
                src_addr_set = 1;
                break;
            }
#ifdef IPV6
            if (inet_pton(AF_INET6, optparse_state.optarg, &src_addr6)) {
                src_addr6_set = 1;
                break;
            }
#endif
            fprintf(stderr, "%s: can't parse source address: %s\n", prog, optparse_state.optarg);
            exit(1);

        case 'I':
            opt_bindiface_on = 1;
#ifdef SO_BINDTODEVICE
            if (socket4 >= 0) {
                if (p_setsockopt(suid, socket4, SOL_SOCKET, SO_BINDTODEVICE, optparse_state.optarg, strlen(optparse_state.optarg))) {
                    perror("binding to specific interface (SO_BINDTODEVICE)");
                    exit(1);
                }
            }
#ifdef IPV6
            if (socket6 >= 0) {
                if (p_setsockopt(suid, socket6, SOL_SOCKET, SO_BINDTODEVICE, optparse_state.optarg, strlen(optparse_state.optarg))) {
                    perror("binding to specific interface (SO_BINDTODEVICE), IPV6");
                    exit(1);
                }
            }
#endif
#else
            printf("%s: cant bind to a particular net interface since SO_BINDTODEVICE is not supported on your os.\n", prog);
            exit(3);
            ;
#endif
            break;

        case 'J':
            opt_print_json_on = 1;
            break;

        case 'T':
            /* This option is ignored for compatibility reasons ("select timeout" is not meaningful anymore) */
            break;

        case 'O':
            {
                unsigned long val = strtoul_strict(optparse_state.optarg, 0);
                if (val > 255)
                    usage(1);
                tos = (int)val;
            }
            if (socket4 >= 0) {
                if (setsockopt(socket4, IPPROTO_IP, IP_TOS, &tos, sizeof(tos))) {
                    perror("setting type of service octet IP_TOS");
                }
            }
#if defined(IPV6) && defined(IPV6_TCLASS)
            if (socket6 >= 0) {
                if (setsockopt(socket6, IPPROTO_IPV6, IPV6_TCLASS, &tos, sizeof(tos))) {
                    perror("setting type of service octet IPV6_TCLASS");
                }
            }
#endif
            break;

        case 'o':
            opt_outage_on = 1;
            break;

        case '?':
            fprintf(stderr, "%s: %s\n", argv[0], optparse_state.errmsg);
            fprintf(stderr, "see 'fping -h' for usage information\n");
            exit(1);
            break;
        }
    }

    /* permanently drop privileges */
    if (suid != getuid() && setuid(getuid())) {
        perror("fatal: failed to permanently drop privileges");
        /* continuing would be a security hole */
        exit(4);
    }

    /* validate various option settings */

#ifndef IPV6
    if (socket4 < 0) {
        crash_and_burn("can't create socket (must run as root?)");
    }
#else
    if ((socket4 < 0 && socket6 < 0) || (hints_ai_family == AF_INET6 && socket6 < 0)) {
        crash_and_burn("can't create socket (must run as root?)");
    }
#endif

    if (opt_ttl > 255) {
        fprintf(stderr, "%s: ttl %u out of range\n", prog, opt_ttl);
        exit(1);
    }

    if (opt_unreachable_on && opt_alive_on) {
        fprintf(stderr, "%s: specify only one of a, u\n", prog);
        exit(1);
    }

    if (opt_oiface_on && opt_bindiface_on) {
        fprintf(stderr, "%s: specify only --oiface or -I, --iface\n", prog);
        exit(1);
    }

    if (opt_count_on && opt_loop_on) {
        fprintf(stderr, "%s: specify only one of c, l\n", prog);
        exit(1);
    }

    if (opt_print_json_on && !opt_count_on && !opt_loop_on) {
        fprintf(stderr, "%s: option -J, --json requires -c, -C, or -l\n", prog);
        exit(1);
    }

    if (opt_interval < (float)MIN_INTERVAL_MS * 1000000 && getuid()) {
        fprintf(stderr, "%s: -i must be >= %g\n", prog, (float)MIN_INTERVAL_MS);
        exit(1);
    }

    if (opt_perhost_interval < (float)MIN_PERHOST_INTERVAL_MS * 1000000 && getuid()) {
        fprintf(stderr, "%s: -p must be >= %g\n", prog, (float)MIN_PERHOST_INTERVAL_MS);
        exit(1);
    }

    if (opt_ping_data_size > MAX_PING_DATA) {
        fprintf(stderr, "%s: data size %u not valid, must not be larger than %u\n",
            prog, opt_ping_data_size, (unsigned int)MAX_PING_DATA);
        exit(1);
    }

    if ((opt_backoff > MAX_BACKOFF_FACTOR) || (opt_backoff < MIN_BACKOFF_FACTOR)) {
        fprintf(stderr, "%s: backoff factor %.1f not valid, must be between %.1f and %.1f\n",
            prog, opt_backoff, MIN_BACKOFF_FACTOR, MAX_BACKOFF_FACTOR);
        exit(1);
    }

    if (opt_icmp_request_typ == 13 && opt_size_on != 0) {
        fprintf(stderr, "%s: cannot change ICMP Timestamp size\n", prog);
        exit(1);
    }

    if (opt_count_on) {
        if (opt_verbose_on)
            opt_per_recv_on = 1;

        opt_alive_on = opt_unreachable_on = opt_verbose_on = 0;
    }

    if (opt_loop_on) {
        if (!report_interval)
            opt_per_recv_on = 1;

        opt_alive_on = opt_unreachable_on = opt_verbose_on = 0;
    }

    if (opt_alive_on || opt_unreachable_on || opt_min_reachable)
        opt_verbose_on = 0;

    trials = (opt_count > opt_retry + 1) ? opt_count : opt_retry + 1;

    /* auto-tune default timeout for count/loop modes
     * see also github #32 */
    if (opt_loop_on || opt_count_on) {
        if (!opt_timeout_on) {
            opt_timeout = opt_perhost_interval;
            if (opt_timeout > (int64_t)AUTOTUNE_TIMEOUT_MAX * 1000000) {
                opt_timeout = (int64_t)AUTOTUNE_TIMEOUT_MAX * 1000000;
            }
        }
    }

#if defined(DEBUG) || defined(_DEBUG)
    if (debugging & DBG_TRACE)
        opt_debug_trace_on = 1;

    if (debugging & DBG_RANDOM_LOSE_FEW) {
        opt_debug_randomly_lose_on = 1;
        lose_factor = 1; /* ie, 1/4 */
    }

    if (debugging & DBG_RANDOM_LOSE_MANY) {
        opt_debug_randomly_lose_on = 1;
        lose_factor = 5; /* ie, 3/4 */
    }

    if (debugging & DBG_PRINT_PER_SYSTEM)
        opt_debug_print_per_system_on = 1;

    if ((debugging & DBG_REPORT_ALL_RTTS) && !opt_loop_on)
        opt_report_all_rtts_on = 1;

    if (opt_debug_trace_on) {
        fprintf(stderr, "%s:\n  opt_count: %u, opt_retry: %u, opt_interval: %.0f ms\n",
            prog, opt_count, opt_retry, opt_interval / 1e6);
        fprintf(stderr, "  opt_perhost_interval: %.0f ms, opt_timeout: %.0f\n",
            opt_perhost_interval / 1e6, opt_timeout / 1e6);
        fprintf(stderr, "  opt_seqmap_timeout: %.0f\n", opt_seqmap_timeout / 1e6);
        fprintf(stderr, "  opt_ping_data_size = %u, trials = %u\n",
            opt_ping_data_size, trials);

        if (opt_verbose_on)
            fprintf(stderr, "  opt_verbose_on set\n");
        if (opt_multif_on)
            fprintf(stderr, "  opt_multif_on set\n");
        if (opt_name_on)
            fprintf(stderr, "  opt_name_on set\n");
        if (opt_addr_on)
            fprintf(stderr, "  opt_addr_on set\n");
        if (opt_stats_on)
            fprintf(stderr, "  opt_stats_on set\n");
        if (opt_unreachable_on)
            fprintf(stderr, "  opt_unreachable_on set\n");
        if (opt_alive_on)
            fprintf(stderr, "  opt_alive_on set\n");
        if (opt_elapsed_on)
            fprintf(stderr, "  opt_elapsed_on set\n");
        if (opt_version_on)
            fprintf(stderr, "  opt_version_on set\n");
        if (opt_count_on)
            fprintf(stderr, "  opt_count_on set\n");
        if (opt_loop_on)
            fprintf(stderr, "  opt_loop_on set\n");
        if (opt_backoff_on)
            fprintf(stderr, "  opt_backoff_on set\n");
        if (opt_per_recv_on)
            fprintf(stderr, "  opt_per_recv_on set\n");
        if (opt_report_all_rtts_on)
            fprintf(stderr, "  opt_report_all_rtts_on set\n");
        if (opt_debug_randomly_lose_on)
            fprintf(stderr, "  opt_debug_randomly_lose_on set\n");
        if (opt_debug_print_per_system_on)
            fprintf(stderr, "  opt_debug_print_per_system_on set\n");
        if (opt_outage_on)
            fprintf(stderr, "  opt_outage_on set\n");
        if (opt_print_netdata_on)
            fprintf(stderr, "  opt_print_netdata_on set\n");
        if (opt_print_json_on)
            fprintf(stderr, "  opt_print_json_on set\n");
    }
#endif /* DEBUG || _DEBUG */

    /* set the TTL, if the -H option was set (otherwise ttl will be = 0) */
    if (opt_ttl > 0) {
        if (socket4 >= 0) {
            if (setsockopt(socket4, IPPROTO_IP, IP_TTL, &opt_ttl, sizeof(opt_ttl))) {
                perror("setting time to live");
            }
        }
#ifdef IPV6
        if (socket6 >= 0) {
            if (setsockopt(socket6, IPPROTO_IPV6, IPV6_UNICAST_HOPS, &opt_ttl, sizeof(opt_ttl))) {
                perror("setting time to live");
            }
        }
#endif
    }

#if HAVE_SO_TIMESTAMPNS
    {
        int opt = 1;
        if (socket4 >= 0) {
            if (setsockopt(socket4, SOL_SOCKET, SO_TIMESTAMPNS, &opt, sizeof(opt))) {
                if (setsockopt(socket4, SOL_SOCKET, SO_TIMESTAMP, &opt, sizeof(opt))) {
                    perror("setting SO_TIMESTAMPNS and SO_TIMESTAMP option");
                }
            }
        }
#ifdef IPV6
        if (socket6 >= 0) {
            if (setsockopt(socket6, SOL_SOCKET, SO_TIMESTAMPNS, &opt, sizeof(opt))) {
                if (setsockopt(socket6, SOL_SOCKET, SO_TIMESTAMP, &opt, sizeof(opt))) {
                    perror("setting SO_TIMESTAMPNS and SO_TIMESTAMP option (IPv6)");
                }
            }
        }
#endif
    }
#endif

    update_current_time();
    start_time = current_time_ns;

    /* handle host names supplied on command line or in a file */
    /* if the opt_generate_on is on, then generate the IP list */

    argv = &argv[optparse_state.optind];
    argc -= optparse_state.optind;

    /* calculate how many ping can be in-flight per host */
    if (opt_count_on) {
        event_storage_count = opt_count;
    }
    else if (opt_loop_on) {
        if (opt_perhost_interval > opt_timeout) {
            event_storage_count = 1;
        }
        else {
            event_storage_count = 1 + opt_timeout / opt_perhost_interval;
        }
    }
    else {
        event_storage_count = 1;
    }

    /* file and generate are mutually exclusive */
    /* file and command line are mutually exclusive */
    /* generate requires command line parameters beyond the switches */
    if ((*argv && filename) || (filename && opt_generate_on) || (opt_generate_on && !*argv))
        usage(1);

    /* if no conditions are specified, then assume input from stdin */
    if (!*argv && !filename && !opt_generate_on)
        filename = "-";

    if (*argv && !opt_generate_on) {
        while (*argv) {
            add_name(*argv);
            ++argv;
        }
    }
    else if (filename) {
        FILE *ping_file;
        char line[MAX_TARGET_NAME_LEN + 1];
        char host[MAX_TARGET_NAME_LEN + 1];
        char scratch[MAX_TARGET_NAME_LEN + 1];
        int skip, non_empty;

        if (strcmp(filename, "-") == 0)
            ping_file = fdopen(0, "r");
        else
            ping_file = fopen(filename, "r");

        if (!ping_file)
            errno_crash_and_burn("fopen");

        /*
         * Read the first word of every non-comment line, skip everything else.
         * (Empty and blank lines are ignored.  Lines where the first non-blank
         * character is a '#' are interpreted as comments and ignored.)
        */
        while (fgets(line, sizeof(line), ping_file)) {
            skip = non_empty = 0;

            /* skip over a prefix of the line where sscanf finds nothing */
            if ((sscanf(line, "%s", host) != 1) || (!*host)) {
                continue;
            }

            /* the first word of the line can indicate a comment line */
            if (host[0] == '#') {
                skip = 1; /* skip remainder of line */
            } else {
                non_empty = 1; /* we have something to add as a target name */
                /*
                 * We have found the start of a word.
                 * This part of the line may contain all of the first word.
                 */
                if (!strchr(line, '\n') && (strlen(line) == sizeof(line) - 1)) {
                    char discard1[MAX_TARGET_NAME_LEN + 1];
                    char discard2[MAX_TARGET_NAME_LEN + 1];
                    if (sscanf(line, "%s%s", discard1, discard2) == 2) {
                        skip = 1; /* a second word starts in this part */
                    }
                    if (isspace(line[sizeof(line) - 2])) {
                        skip = 1; /* the first word ends in this part */
                    }
                }
            }
            /* read remainder of this input line */
            while (!strchr(line, '\n') && fgets(line, sizeof(line), ping_file)) {
                if (skip) {
                    continue; /* skip rest of data in this input line */
                }
                if (isspace(line[0])) {
                    skip = 1; /* first word ended in previous part */
                    continue;
                }
                if ((sscanf(line, "%s", scratch) != 1) || (!*scratch)) {
                    skip = 1; /* empty or blank part of line, skip the rest */
                    continue;
                }
                if (sizeof(host) - strlen(host) < strlen(scratch) + 1) {
                    fprintf(stderr, "%s: target name too long\n", prog);
                    exit(1);
                }
                /* append remainder of word started in previous line part */
                strncat(host, scratch, sizeof(host) - strlen(host) - 1);
                /*
                 * Since the "host" buffer is the same size as the "line"
                 * buffer, a target name that fits into the "host" buffer
                 * cannot use more than two consecutive line parts.
                 * A target name that uses two consecutive line parts
                 * and fits into the "host" buffer must end before the
                 * end of the second "line" buffer.  Thus the rest of
                 * the line can be skipped.
                 */
                skip = 1;
            }

            if (non_empty)
                add_name(host);
        }

        fclose(ping_file);
    }
    else if (*argv && opt_generate_on) {
        if (argc == 1) {
            /* one target: we expect a cidr range (n.n.n.n/m) */
            add_cidr(argv[0]);
        }
        else if (argc == 2) {
            add_range(argv[0], argv[1]);
        }
        else {
            usage(1);
        }
    }
    else {
        usage(1);
    }

    if (!num_hosts) {
        exit(num_noaddress ? 2 : 1);
    }

    if (socket4 >= 0 && (src_addr_set || socktype4 == SOCK_DGRAM)) {
        socket_set_src_addr_ipv4(socket4, &src_addr, (socktype4 == SOCK_DGRAM) ? &ident4 : NULL);
    }
#ifdef IPV6
    if (socket6 >= 0 && (src_addr6_set || socktype6 == SOCK_DGRAM)) {
        socket_set_src_addr_ipv6(socket6, &src_addr6, (socktype6 == SOCK_DGRAM) ? &ident6 : NULL);
    }
#endif

    /* allocate and initialize array to map host nr to host_entry */
    {
        struct event *cursor = event_queue_ping.first;
        int i = 0;
        table = (HOST_ENTRY **)calloc(num_hosts, sizeof(HOST_ENTRY *));
        if (!table)
            crash_and_burn("Can't malloc array of hosts");
        /* initialize table of hosts. we know that we have ping events scheduled
         * for each of them */
        for (cursor = event_queue_ping.first; cursor; cursor = cursor->ev_next) {
            table[i] = cursor->host;
            cursor->host->i = i;
            i++;
        }
    }

    init_ping_buffer_ipv4(opt_ping_data_size);
#ifdef IPV6
    init_ping_buffer_ipv6(opt_ping_data_size);
#endif

#ifdef USE_SIGACTION
    memset(&act, 0, sizeof(act));
    act.sa_handler = signal_handler;
    sigemptyset(&act.sa_mask);
    sigaddset(&act.sa_mask, SIGINT);
    sigaddset(&act.sa_mask, SIGQUIT);
    act.sa_flags = SA_RESTART;
    if (sigaction(SIGQUIT, &act, NULL) || sigaction(SIGINT, &act, NULL)) {
        crash_and_burn("failure to set signal handler");
    }
#else
    signal(SIGINT, signal_handler);
    signal(SIGQUIT, signal_handler);
#endif
    setlinebuf(stdout);

    // Last time we updated current_time_ns was before adding the hosts and
    // possibly doing DNS resolution, which means that it isn't accurate
    // anymore.
    update_current_time();
    if (report_interval) {
        next_report_time = current_time_ns + report_interval;
    }

    last_send_time = 0;

    seqmap_init(opt_seqmap_timeout);

    /* main loop */
    main_loop();

/* Debug: CPU Performance */
#if defined(DEBUG) || defined(_DEBUG)
    perf_cpu_end = clock();
    perf_cpu_time_used = ((double) (perf_cpu_end - perf_cpu_start)) / CLOCKS_PER_SEC;
    printf("[DEBUG] CPU time used: %f sec\n", perf_cpu_time_used);
#endif /* DEBUG || _DEBUG */

    finish();

    return 0;
}

static inline int64_t timespec_ns(struct timespec *a)
{
    return ((int64_t)a->tv_sec * 1000000000) + a->tv_nsec;
}

#if HAVE_SO_TIMESTAMPNS
/* convert a struct timeval to nanoseconds */
static inline int64_t timeval_ns(struct timeval *a)
{
    return ((int64_t)a->tv_sec * 1000000000) + ((int64_t)a->tv_usec * 1000);
}
#endif /* HAVE_SO_TIMESTAMPNS */

void add_cidr(char *addr)
{
    char *addr_end;
    char *mask_str;
    unsigned long mask;
    int ret;
    struct addrinfo addr_hints;
    struct addrinfo *addr_res;
    unsigned long net_addr;
#ifdef IPV6
    uint64_t net_upper, net_lower;
    char *scope_str;
#endif /* IPV6 */

    /* Split address from mask */
    addr_end = strrchr(addr, '/');
    if (addr_end == NULL) {
        usage(1);
    }
    mask_str = addr_end + 1;

#ifdef IPV6
    /* IPv6 addresses can have a scope */
    scope_str = strchr(addr, '%');
    if (scope_str && mask_str < scope_str) {
        fprintf(stderr, "%s: address scope must precede prefix length\n", prog);
        exit(1);
    }
#endif /*IPV6 */

    *addr_end = '\0';
    mask = strtoul_strict(mask_str, 10);

    /* parse address */
    memset(&addr_hints, 0, sizeof(struct addrinfo));
    addr_hints.ai_family = hints_ai_family;
    addr_hints.ai_flags = AI_NUMERICHOST;
    ret = getaddrinfo(addr, NULL, &addr_hints, &addr_res);
    if (ret) {
        fprintf(stderr, "%s, can't parse address %s: %s\n", prog, addr, gai_strerror(ret));
        exit(1);
    }
    if (addr_res->ai_family == AF_INET) {
        net_addr = ntohl(((struct sockaddr_in*)addr_res->ai_addr)->sin_addr.s_addr);
        freeaddrinfo(addr_res);
        add_cidr_ipv4(net_addr, mask);
#ifdef IPV6
    } else if (addr_res->ai_family == AF_INET6) {
        uint8_t *ipv6_addr = ((struct sockaddr_in6*)addr_res->ai_addr)->sin6_addr.s6_addr;
        net_upper = be_octets_to_uint64(ipv6_addr);
        net_lower = be_octets_to_uint64(ipv6_addr + 8);
        freeaddrinfo(addr_res);
        add_cidr_ipv6(net_upper, net_lower, mask, scope_str);
#endif /* IPV6 */
    } else {
        freeaddrinfo(addr_res);
        fprintf(stderr, "%s: -g does not support this address family\n", prog);
        exit(1);
    }
}

void add_cidr_ipv4(unsigned long net_addr, unsigned long mask)
{
    unsigned long bitmask;
    unsigned long net_last;

    /* check mask */
    if (mask < 1 || mask > 32) {
        fprintf(stderr, "%s: netmask must be between 1 and 32 (is: %lu)\n", prog, mask);
        exit(1);
    }

    /* convert mask integer from 1 to 32 to a bitmask */
    bitmask = ((unsigned long)0xFFFFFFFF) << (32 - mask);

    /* calculate network range */
    net_addr &= bitmask;
    net_last = net_addr + ((unsigned long)0x1 << (32 - mask)) - 1;

    /* exclude network and broadcast address for regular prefixes */
    if (mask < 31) {
        net_last--;
        net_addr++;
    }

    /* add all hosts in that network (net_addr and net_last inclusive) */
    add_addr_range_ipv4(net_addr, net_last);
}

#ifdef IPV6
void add_cidr_ipv6(uint64_t net_upper, uint64_t net_lower, unsigned long mask, const char *scope_str)
{
    uint64_t bitmask_lower;
    uint64_t last_lower;

    /* check mask -- 2^63 addresses should suffice for now */
    if (mask < 65 || mask > 128) {
        fprintf(stderr, "%s: netmask must be between 65 and 128 (is: %lu)\n", prog, mask);
        exit(1);
    }

    /* convert mask integer from 65 to 128 to the lower part of a bitmask */
    bitmask_lower = ((uint64_t)-1) << (128 - mask);

    /* calculate network range */
    net_lower &= bitmask_lower;
    last_lower = net_lower + ((uint64_t)1 << (128 - mask)) - 1;

    add_addr_range_ipv6(net_upper, net_lower, net_upper, last_lower, scope_str);
}
#endif /* IPV6 */

void add_range(char *start, char *end)
{
    struct addrinfo addr_hints;
    struct addrinfo *addr_res;
    unsigned long start_long;
    unsigned long end_long;
    int ret;
#ifdef IPV6
    uint64_t start_upper, start_lower;
    uint64_t end_upper, end_lower;
    char *start_scope_str, *end_scope_str;

    /*
     * The compiler does not know that setting the address family hint to
     * ensure that start and end are from the same address family also
     * ensures that either start_long and end_long are initialized and used,
     * or start_upper, start_lower, end_upper, and end_lower are initialized
     * and used.  Thus initialize all variables when both IPv4 and IPv6 are
     * supported to suppress compiler warnings.
     */
    start_long = -1;
    end_long = 0;
    start_upper = start_lower = -1;
    end_upper = end_lower = 0;
    start_scope_str = end_scope_str = NULL;
#endif /* IPV6 */

    /* parse start address */
    memset(&addr_hints, 0, sizeof(struct addrinfo));
    addr_hints.ai_family = hints_ai_family;
    addr_hints.ai_flags = AI_NUMERICHOST;
    ret = getaddrinfo(start, NULL, &addr_hints, &addr_res);
    if (ret) {
        fprintf(stderr, "%s: can't parse address %s: %s\n", prog, start, gai_strerror(ret));
        exit(1);
    }
    /* start and end must be from the same address family */
    hints_ai_family = addr_res->ai_family;
    if (addr_res->ai_family == AF_INET) {
        start_long = ntohl(((struct sockaddr_in*)addr_res->ai_addr)->sin_addr.s_addr);
        freeaddrinfo(addr_res);
#ifdef IPV6
    } else if (addr_res->ai_family == AF_INET6) {
        uint8_t *ipv6_addr = ((struct sockaddr_in6*)addr_res->ai_addr)->sin6_addr.s6_addr;
        start_upper = be_octets_to_uint64(ipv6_addr);
        start_lower = be_octets_to_uint64(ipv6_addr + 8);
        freeaddrinfo(addr_res);
#endif /* IPV6 */
    } else {
        freeaddrinfo(addr_res);
        fprintf(stderr, "%s: -g does not support this address family\n", prog);
        exit(1);
    }

#ifdef IPV6
    /* IPv6 addresses can have a scope */
    if (hints_ai_family == AF_INET6) {
        start_scope_str = strchr(start, '%');
        end_scope_str = strchr(end, '%');
        if ((!start_scope_str && end_scope_str) ||
            (start_scope_str && !end_scope_str) ||
            (start_scope_str && end_scope_str && strcmp(start_scope_str, end_scope_str) != 0)) {
                fprintf(stderr, "%s: different scopes for start and end addresses\n", prog);
                exit(1);
        }
    }
#endif

    /* parse end address */
    memset(&addr_hints, 0, sizeof(struct addrinfo));
    addr_hints.ai_family = hints_ai_family;
    addr_hints.ai_flags = AI_NUMERICHOST;
    ret = getaddrinfo(end, NULL, &addr_hints, &addr_res);
    if (ret) {
        fprintf(stderr, "%s: can't parse address %s: %s\n", prog, end, gai_strerror(ret));
        exit(1);
    }
    if (addr_res->ai_family == AF_INET) {
        end_long = ntohl(((struct sockaddr_in*)addr_res->ai_addr)->sin_addr.s_addr);
        freeaddrinfo(addr_res);
        add_addr_range_ipv4(start_long, end_long);
#ifdef IPV6
    } else if (addr_res->ai_family == AF_INET6) {
        uint8_t *ipv6_addr = ((struct sockaddr_in6*)addr_res->ai_addr)->sin6_addr.s6_addr;
        end_upper = be_octets_to_uint64(ipv6_addr);
        end_lower = be_octets_to_uint64(ipv6_addr + 8);
        freeaddrinfo(addr_res);
        add_addr_range_ipv6(start_upper, start_lower, end_upper, end_lower, start_scope_str);
#endif /* IPV6 */
    } else {
        freeaddrinfo(addr_res);
        fprintf(stderr, "%s: -g does not support this address family\n", prog);
        exit(1);
    }
}

void add_addr_range_ipv4(unsigned long start_long, unsigned long end_long)
{
    /* check if generator limit is exceeded */
    if (end_long >= start_long + MAX_GENERATE) {
        fprintf(stderr, "%s: -g parameter generates too many addresses\n", prog);
        exit(1);
    }

    /* generate */
    for (; start_long <= end_long; start_long++) {
        struct in_addr in_addr_tmp;
        char buffer[20];
        in_addr_tmp.s_addr = htonl(start_long);
        inet_ntop(AF_INET, &in_addr_tmp, buffer, sizeof(buffer));
        add_name(buffer);
    }
}

#ifdef IPV6
uint64_t be_octets_to_uint64(uint8_t *be_octets)
{
    int i;
    uint64_t ret = 0;
    for (i = 0; i < 8; i++) {
        ret |= (uint64_t)be_octets[7 - i] << (i * 8);
    }
    return ret;
}

void uint64_to_be_octets(uint64_t num, uint8_t *be_octets)
{
    int i;
    for (i = 0; i < 8; i++) {
        be_octets[7 - i] = (uint8_t)((num >> (i * 8)) & 0xff);
    }
}

void add_addr_range_ipv6(uint64_t start_upper, uint64_t start_lower,
                         uint64_t end_upper, uint64_t end_lower,
                         const char *scope_str)
{
    struct in6_addr in6_addr_tmp;
    char buffer[100];

    /* prevent generating too many addresses */
    if ((start_upper + 1 < end_upper) ||
        (start_upper + 1 == end_upper && end_lower >= start_lower) ||
        (start_upper + 1 == end_upper && end_lower - MAX_GENERATE >= start_lower) ||
        (start_upper == end_upper && end_lower - MAX_GENERATE >= start_lower &&
                                     start_lower + MAX_GENERATE <= end_lower)) {
        fprintf(stderr, "%s: -g parameter generates too many addresses\n", prog);
        exit(1);
    }

    while ((start_upper < end_upper) ||
           (start_upper == end_upper && start_lower <= end_lower)) {
        uint64_to_be_octets(start_upper, in6_addr_tmp.s6_addr);
        uint64_to_be_octets(start_lower, in6_addr_tmp.s6_addr + 8);
        inet_ntop(AF_INET6, &in6_addr_tmp, buffer, sizeof(buffer));
        if (scope_str) {
	    if (strlen(buffer) + strlen(scope_str) + 1 > sizeof(buffer)) {
                fprintf(stderr, "%s: scope identifier is too long\n", prog);
                exit(1);
            }
            strncat(buffer, scope_str, sizeof(buffer) - strlen(buffer) - 1);
        }
        add_name(buffer);
        start_lower++;
        if (start_lower == 0) {
            start_upper++;
        }
    }
}
#endif /* IPv6 */

void main_loop()
{
    int64_t lt;
    int64_t wait_time_ns;
    struct event *event;
    struct host_entry *h;

    while (event_queue_ping.first || event_queue_timeout.first) {
        dbg_printf("%s", "# main_loop\n");

        /* timeout event ? */
        if (event_queue_timeout.first && event_queue_timeout.first->ev_time - current_time_ns <= 0) {
            event = ev_dequeue(&event_queue_timeout);
            h = event->host;

            dbg_printf("%s [%d]: timeout event\n", h->host, event->ping_index);

            stats_add(h, event->ping_index, 0, -1);

            if (opt_per_recv_on) {
                print_timeout(h, event->ping_index);
            }

            /* do we need to send a retry? */
            if (!opt_loop_on && !opt_count_on) {
                if (h->num_sent < opt_retry + 1) {
                    if (opt_backoff_on) {
                        h->timeout *= opt_backoff;
                    }
                    send_ping(h, event->ping_index);
                }
            }

            /* note: we process first timeout events, because we might need to
             * wait to process ping events, while we for sure never need to
             * wait for timeout events.
             */
            continue;
        }

        /* ping event ? */
        if (event_queue_ping.first && event_queue_ping.first->ev_time - current_time_ns <= 0) {
            /* Make sure that we don't ping more than once every "interval" */
            lt = current_time_ns - last_send_time;
            if (lt < opt_interval)
                goto wait_for_reply;

            /* Dequeue the event */
            event = ev_dequeue(&event_queue_ping);
            h = event->host;

            dbg_printf("%s [%d]: ping event\n", h->host, event->ping_index);

            /* Send the ping */
            send_ping(h, event->ping_index);

            /* Loop and count mode: schedule next ping */
            if (opt_loop_on || (opt_count_on && event->ping_index + 1 < opt_count)) {
                host_add_ping_event(h, event->ping_index + 1, event->ev_time + opt_perhost_interval);
            }
        }

    wait_for_reply:

        /* When is the next ping next event? */
        wait_time_ns = -1;
        if (event_queue_ping.first) {
            wait_time_ns = event_queue_ping.first->ev_time - current_time_ns;
            if (wait_time_ns < 0)
                wait_time_ns = 0;
            /* make sure that we wait enough, so that the inter-ping delay is
             * bigger than 'interval' */
            if (wait_time_ns < opt_interval) {
                lt = current_time_ns - last_send_time;
                if (lt < opt_interval) {
                    wait_time_ns = opt_interval - lt;
                }
            }

            dbg_printf("next ping event in %.0f ms (%s)\n", wait_time_ns / 1e6, event_queue_ping.first->host->host);
        }

        /* When is the next timeout event? */
        if (event_queue_timeout.first) {
            int64_t wait_time_timeout = event_queue_timeout.first->ev_time - current_time_ns;
            if (wait_time_ns < 0 || wait_time_timeout < wait_time_ns) {
                wait_time_ns = wait_time_timeout;
                if (wait_time_ns < 0) {
                    wait_time_ns = 0;
                }
            }

            dbg_printf("next timeout event in %.0f ms (%s)\n", wait_time_timeout / 1e6, event_queue_timeout.first->host->host);
        }

        /* When is the next report due? */
        if (report_interval && (opt_loop_on || opt_count_on)) {
            int64_t wait_time_next_report = next_report_time - current_time_ns;
            if (wait_time_next_report < wait_time_ns) {
                wait_time_ns = wait_time_next_report;
                if (wait_time_ns < 0) {
                    wait_time_ns = 0;
                }
            }

            dbg_printf("next report  event in %0.f ms\n", wait_time_next_report / 1e6);
        }

        /* if wait_time is still -1, it means that we are waiting for nothing... */
        if (wait_time_ns == -1) {
            break;
        }

        /* end of loop was requested by interrupt signal handler */
        if (finish_requested) {
            break;
        }

        /* Receive replies */
        /* (this is what sleeps during each loop iteration) */
        dbg_printf("waiting up to %.0f ms\n", wait_time_ns / 1e6);
        if (wait_for_reply(wait_time_ns)) {
            while (wait_for_reply(0))
                ; /* process other replies in the queue */
        }

        update_current_time();

        if (status_snapshot) {
            status_snapshot = 0;
            if (opt_print_json_on)
                print_per_system_splits_json();
            else
                print_per_system_splits();
        }

        /* Print report */
        if (report_interval && (opt_loop_on || opt_count_on) && (current_time_ns >= next_report_time)) {
            if (opt_print_netdata_on) {
                print_netdata();
            }
            else if (opt_print_json_on) {
                print_per_system_splits_json();
            }
            else {
                print_per_system_splits();
            }

            while (current_time_ns >= next_report_time) {
                next_report_time += report_interval;
            }
        }
    }
}

/************************************************************

  Function: signal_handler

*************************************************************

  Inputs:  int signum

  Description:

  SIGQUIT signal handler - set flag and return
  SIGINT signal handler - set flag and return

************************************************************/

void signal_handler(int signum)
{
    switch (signum) {
    case SIGINT:
        finish_requested = 1;
        break;

    case SIGQUIT:
        status_snapshot = 1;
        break;
    }
}

/************************************************************

  Function: update_current_time

*************************************************************/

void update_current_time()
{
    clock_gettime(CLOCKID, &current_time);
    current_time_ns = timespec_ns(&current_time);
}

/************************************************************

  Function: finish

*************************************************************

  Inputs:  void (none)

  Description:

  Main program clean up and exit point

************************************************************/

void finish()
{
    int i;
    HOST_ENTRY *h;

    update_current_time();
    end_time = current_time_ns;

    /* tot up unreachables */
    for (i = 0; i < num_hosts; i++) {
        h = table[i];

        if (!h->num_recv) {
            num_unreachable++;

            if (opt_verbose_on || opt_unreachable_on) {
                printf("%s", h->host);

                if (opt_verbose_on)
                    printf(" is unreachable");

                printf("\n");
            }
        }
    }

    if (opt_count_on || opt_loop_on) {
        if (opt_print_json_on)
            print_per_system_stats_json();
        else
            print_per_system_stats();
    }
#if defined(DEBUG) || defined(_DEBUG)
    else if (opt_debug_print_per_system_on) {
        if (opt_print_json_on)
            print_per_system_stats_json();
        else
            print_per_system_stats();
    }
#endif /* DEBUG || _DEBUG */

    if (opt_stats_on) {
        if (opt_print_json_on)
            print_global_stats_json();
        else
            print_global_stats();
    }

    if (opt_min_reachable) {
        if ((num_hosts - num_unreachable) >= opt_min_reachable) {
            printf("Enough hosts reachable (required: %d, reachable: %d)\n", opt_min_reachable, num_hosts - num_unreachable);
            exit(0);
        }
        else {
            printf("Not enough hosts reachable (required: %d, reachable: %d)\n", opt_min_reachable, num_hosts - num_unreachable);
            exit(1);
        }
    }

    if (num_noaddress)
        exit(2);
    else if (num_alive != num_hosts)
        exit(1);

    exit(0);
}

/************************************************************

  Function: send_ping

*************************************************************

  Inputs:  int s, HOST_ENTRY *h

  Description:

  Compose and transmit an ICMP_ECHO REQUEST packet.  The IP packet
  will be added on by the kernel.  The ID field is our UNIX process ID,
  and the sequence number is an index into an array of outstanding
  ping requests. The sequence number will later be used to quickly
  figure out who the ping reply came from.

************************************************************/

int send_ping(HOST_ENTRY *h, int index)
{
    int n;
    int myseq;
    int ret = 1;
    uint8_t proto = ICMP_ECHO;

    update_current_time();
    h->last_send_time = current_time_ns;
    myseq = seqmap_add(h->i, index, current_time_ns);

    dbg_printf("%s [%d]: send ping\n", h->host, index);

    if (h->saddr.ss_family == AF_INET && socket4 >= 0) {
        if(opt_icmp_request_typ == 13)
            proto = ICMP_TSTAMP;
        n = socket_sendto_ping_ipv4(socket4, (struct sockaddr *)&h->saddr, h->saddr_len, myseq, ident4, proto);
    }
#ifdef IPV6
    else if (h->saddr.ss_family == AF_INET6 && socket6 >= 0) {
        n = socket_sendto_ping_ipv6(socket6, (struct sockaddr *)&h->saddr, h->saddr_len, myseq, ident6);
    }
#endif
    else {
        return 0;
    }

    /* error sending? */
    if (
        (n < 0)
#if defined(EHOSTDOWN)
        && errno != EHOSTDOWN
#endif
    ) {
        if (opt_verbose_on) {
            print_warning("%s: error while sending ping: %s\n", h->host, strerror(errno));
        }
        else {
            dbg_printf("%s: error while sending ping: %s\n", h->host, strerror(errno));
        }

        h->num_sent++;
        h->num_sent_i++;
        if (!opt_loop_on)
            h->resp_times[index] = RESP_ERROR;

        ret = 0;
    }
    else {
        /* schedule timeout */
        host_add_timeout_event(h, index, current_time_ns + h->timeout);

        /* mark this trial as outstanding */
        if (!opt_loop_on) {
            h->resp_times[index] = RESP_WAITING;
        }
    }

    num_pingsent++;
    last_send_time = h->last_send_time;

    return (ret);
}

int socket_can_read(struct timeval *timeout)
{
    int nfound;
    fd_set readset;
    int socketmax;

#ifndef IPV6
    socketmax = socket4;
#else
    socketmax = socket4 > socket6 ? socket4 : socket6;
#endif

select_again:
    FD_ZERO(&readset);
    if (socket4 >= 0)
        FD_SET(socket4, &readset);
#ifdef IPV6
    if (socket6 >= 0)
        FD_SET(socket6, &readset);
#endif

    nfound = select(socketmax + 1, &readset, NULL, NULL, timeout);
    if (nfound < 0) {
        if (errno == EINTR) {
            /* interrupted system call: redo the select */
            goto select_again;
        }
        else {
            perror("select");
        }
    }

    if (nfound > 0) {
        if (socket4 >= 0 && FD_ISSET(socket4, &readset)) {
            return socket4;
        }
#ifdef IPV6
        if (socket6 >= 0 && FD_ISSET(socket6, &readset)) {
            return socket6;
        }
#endif
    }

    return -1;
}

int receive_packet(int64_t wait_time,
#if HAVE_SO_TIMESTAMPNS
    int64_t *reply_timestamp,
#else
    int64_t *reply_timestamp __attribute__((unused)),
#endif
    struct sockaddr *reply_src_addr,
    size_t reply_src_addr_len,
    char *reply_buf,
    size_t reply_buf_len,
    int *ip_header_tos,
    int *ip_header_ttl)
{
    struct timeval to;
    int s = 0;
    int recv_len;
    static unsigned char msg_control[128];
    struct iovec msg_iov = {
        reply_buf,
        reply_buf_len
    };
    struct msghdr recv_msghdr = {0};
    recv_msghdr.msg_name = reply_src_addr;
    recv_msghdr.msg_namelen = reply_src_addr_len;
    recv_msghdr.msg_iov = &msg_iov;
    recv_msghdr.msg_iovlen = 1;
    recv_msghdr.msg_control = &msg_control;
    recv_msghdr.msg_controllen = sizeof(msg_control);
    struct cmsghdr *cmsg;

    /* Wait for a socket to become ready */
#if HAVE_MSG_DONTWAIT
    if (wait_time == 0) {
        /* Optimization: if wait_time is 0, we can skip select() and just try to
         * read from the sockets with MSG_DONTWAIT */
        recv_len = (socket4 >= 0) ? recvmsg(socket4, &recv_msghdr, MSG_TRUNC | MSG_DONTWAIT) : -1;
#ifdef IPV6
        if (recv_len <= 0 && socket6 >= 0) {
            /* Reset fields potentially modified by failed recvmsg */
            recv_msghdr.msg_namelen = reply_src_addr_len;
            recv_msghdr.msg_controllen = sizeof(msg_control);
            recv_len = recvmsg(socket6, &recv_msghdr, MSG_TRUNC | MSG_DONTWAIT);
        }
#endif
        if (recv_len > 0) {
            goto packet_received;
        }
        return 0;
    }
#endif

    int64_t wait_us = (wait_time + 999) / 1000; // round up (1 ns -> 1 us)
    to.tv_sec = wait_us / 1000000;
    to.tv_usec = wait_us % 1000000;

    s = socket_can_read(&to);
    if (s == -1 || (recv_len = recvmsg(s, &recv_msghdr, MSG_TRUNC)) <= 0) {
        return 0;
    }

packet_received:
    /* ancilliary data */
    {
#if HAVE_SO_TIMESTAMPNS
        struct timespec reply_timestamp_ts;
        struct timeval reply_timestamp_tv;
#endif
        for (cmsg = CMSG_FIRSTHDR(&recv_msghdr);
             cmsg != NULL;
             cmsg = CMSG_NXTHDR(&recv_msghdr, cmsg)) {
#if HAVE_SO_TIMESTAMPNS
            if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_TIMESTAMPNS) {
                memcpy(&reply_timestamp_ts, CMSG_DATA(cmsg), sizeof(reply_timestamp_ts));
                *reply_timestamp = timespec_ns(&reply_timestamp_ts);
            }
            if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_TIMESTAMP) {
                memcpy(&reply_timestamp_tv, CMSG_DATA(cmsg), sizeof(reply_timestamp_tv));
                *reply_timestamp = timeval_ns(&reply_timestamp_tv);
            }
#endif
#if defined(HAVE_IP_RECVTOS)
            if (cmsg->cmsg_level == IPPROTO_IP && cmsg->cmsg_type == IP_TOS) {
                memcpy(ip_header_tos, CMSG_DATA(cmsg), sizeof(*ip_header_tos));
            }
#endif
            if (cmsg->cmsg_level == IPPROTO_IP && cmsg->cmsg_type == IP_TTL) {
                memcpy(ip_header_ttl, CMSG_DATA(cmsg), sizeof(*ip_header_ttl));
            }
#ifdef IPV6
            if (cmsg->cmsg_level == IPPROTO_IPV6 && cmsg->cmsg_type == IPV6_TCLASS) {
                memcpy(ip_header_tos, CMSG_DATA(cmsg), sizeof(*ip_header_tos));
            }
            if (cmsg->cmsg_level == IPPROTO_IPV6 && cmsg->cmsg_type == IPV6_HOPLIMIT) {
                memcpy(ip_header_ttl, CMSG_DATA(cmsg), sizeof(*ip_header_ttl));
            }
#endif
        }
    }

#if defined(DEBUG) || defined(_DEBUG)
    if (opt_debug_randomly_lose_on) {
        if ((random() & 0x07) <= lose_factor)
            return 0;
    }
#endif

    return recv_len;
}

/* stats_add: update host statistics for a single packet that was received (or timed out)
 * h: host entry to update
 * index: if in count mode: index number for this ping packet (-1 otherwise)
 * success: 1 if response received, 0 otherwise
 * latency: response time, in ns
 */
void stats_add(HOST_ENTRY *h, int index, int success, int64_t latency)
{
    /* sent count - we update only on receive/timeout, so that we don't get
     * weird loss percentage, just because a packet was note recived yet.
     */
    h->num_sent++;
    h->num_sent_i++;

    if (!success) {
        if (!opt_loop_on && index >= 0) {
            h->resp_times[index] = RESP_TIMEOUT;
        }
        num_timeout++;
        return;
    }

    /* received count */
    h->num_recv++;
    h->num_recv_i++;

    /* maximum */
    if (!h->max_reply || latency > h->max_reply) {
        h->max_reply = latency;
    }
    if (!h->max_reply_i || latency > h->max_reply_i) {
        h->max_reply_i = latency;
    }

    /* minimum */
    if (!h->min_reply || latency < h->min_reply) {
        h->min_reply = latency;
    }
    if (!h->min_reply_i || latency < h->min_reply_i) {
        h->min_reply_i = latency;
    }

    /* total time (for average) */
    h->total_time += latency;
    h->total_time_i += latency;

    /* response time per-packet (count mode) */
    if (!opt_loop_on && index >= 0) {
        h->resp_times[index] = latency;
    }
}

/* stats_reset_interval: reset interval statistics
 * h: host entry to update
 */
void stats_reset_interval(HOST_ENTRY *h)
{
    h->num_sent_i = 0;
    h->num_recv_i = 0;
    h->max_reply_i = 0;
    h->min_reply_i = 0;
    h->total_time_i = 0;
}

int decode_icmp_ipv4(
    struct sockaddr *response_addr,
    size_t response_addr_len,
    char *reply_buf,
    size_t reply_buf_len,
    unsigned short *id,
    unsigned short *seq,
    IP_HEADER_RESULT *ip_header_res)
{
    struct icmp *icp;
    int hlen = 0;
    struct ip *ip = NULL;

    if (!using_sock_dgram4) {
        ip = (struct ip *)reply_buf;
        ip_header_res->tos = ip->ip_tos;
        ip_header_res->ttl = ip->ip_ttl;

#if defined(__alpha__) && __STDC__ && !defined(__GLIBC__) && !defined(__NetBSD__) && !defined(__OpenBSD__)
        /* The alpha headers are decidedly broken.
         * Using an ANSI compiler, it provides ip_vhl instead of ip_hl and
         * ip_v.  So, to get ip_hl, we mask off the bottom four bits.
         */
        hlen = (ip->ip_vhl & 0x0F) << 2;
#else
        hlen = ip->ip_hl << 2;
#endif
    }

    if (reply_buf_len < hlen + ICMP_MINLEN) {
        /* too short */
        if (opt_verbose_on) {
            char buf[INET6_ADDRSTRLEN];
            getnameinfo(response_addr, response_addr_len, buf, INET6_ADDRSTRLEN, NULL, 0, NI_NUMERICHOST);
            printf("received packet too short for ICMP (%d bytes from %s)\n", (int)reply_buf_len, buf);
        }
        return -1;
    }

    icp = (struct icmp *)(reply_buf + hlen);

    if ((opt_icmp_request_typ == 0 && icp->icmp_type != ICMP_ECHOREPLY) ||
        (opt_icmp_request_typ == 13 && icp->icmp_type != ICMP_TSTAMPREPLY)) {
        /* Handle other ICMP packets */
        struct icmp *sent_icmp;
        SEQMAP_VALUE *seqmap_value;
        char addr_ascii[INET6_ADDRSTRLEN];
        HOST_ENTRY *h;

        /* reply icmp packet (hlen + ICMP_MINLEN) followed by "sent packet" (ip + icmp headers) */
        if (reply_buf_len < hlen + ICMP_MINLEN + sizeof(struct ip) + ICMP_MINLEN) {
            /* discard ICMP message if we can't tell that it was caused by us (i.e. if the "sent packet" is not included). */
            return -1;
        }

        sent_icmp = (struct icmp *)(reply_buf + hlen + ICMP_MINLEN + sizeof(struct ip));

        if ((opt_icmp_request_typ == 0 && sent_icmp->icmp_type != ICMP_ECHO) ||
            (opt_icmp_request_typ == 13 && sent_icmp->icmp_type != ICMP_TSTAMP) ||
            sent_icmp->icmp_id != ident4) {
            /* not caused by us */
            return -1;
        }

        seqmap_value = seqmap_fetch(ntohs(sent_icmp->icmp_seq), current_time_ns);
        if (seqmap_value == NULL) {
            return -1;
        }

        getnameinfo(response_addr, response_addr_len, addr_ascii, INET6_ADDRSTRLEN, NULL, 0, NI_NUMERICHOST);

        switch (icp->icmp_type) {
        case ICMP_UNREACH:
            h = table[seqmap_value->host_nr];
            if (icp->icmp_code > ICMP_UNREACH_MAXTYPE) {
                print_warning("ICMP Unreachable (Invalid Code) from %s for ICMP Echo sent to %s",
                    addr_ascii, h->host);
            }
            else {
                print_warning("%s from %s for ICMP Echo sent to %s",
                    icmp_unreach_str[icp->icmp_code], addr_ascii, h->host);
            }

            print_warning("\n");
            num_othericmprcvd++;
            break;

        case ICMP_SOURCEQUENCH:
        case ICMP_REDIRECT:
        case ICMP_TIMXCEED:
        case ICMP_PARAMPROB:
            h = table[seqmap_value->host_nr];
            if (icp->icmp_type <= ICMP_TYPE_STR_MAX) {
                print_warning("%s from %s for ICMP Echo sent to %s",
                    icmp_type_str[icp->icmp_type], addr_ascii, h->host);
            }
            else {
                print_warning("ICMP %d from %s for ICMP Echo sent to %s",
                    icp->icmp_type, addr_ascii, h->host);
            }
            print_warning("\n");
            num_othericmprcvd++;
            break;
        }

        return -1;
    }

    *id = icp->icmp_id;
    *seq = ntohs(icp->icmp_seq);
    if(icp->icmp_type == ICMP_TSTAMPREPLY) {

        /* Check that reply_buf_len is sufficiently big to contain the timestamps */
        if (reply_buf_len < hlen + ICMP_MINLEN + ICMP_TIMESTAMP_DATA_SIZE) {
            if (opt_verbose_on) {
                char buf[INET6_ADDRSTRLEN];
                getnameinfo(response_addr, response_addr_len, buf, INET6_ADDRSTRLEN, NULL, 0, NI_NUMERICHOST);
                printf("received packet too short for ICMP Timestamp Reply (%d bytes from %s)\n", (int)reply_buf_len, buf);
            }
            return -1;
        }

        ip_header_res->otime_ms = ntohl(icp->icmp_dun.id_ts.its_otime);
        ip_header_res->rtime_ms = ntohl(icp->icmp_dun.id_ts.its_rtime);
        ip_header_res->ttime_ms = ntohl(icp->icmp_dun.id_ts.its_ttime);
    }

    if (opt_print_srcaddr_on) {
        if (ip == NULL || inet_ntop(AF_INET, &ip->ip_dst, ip_header_res->src_addr, sizeof(ip_header_res->src_addr)) == NULL) {
            strncpy(ip_header_res->src_addr, "unknown", sizeof(ip_header_res->src_addr) - 1);
            ip_header_res->src_addr[sizeof(ip_header_res->src_addr) - 1] = '\0';
        }
    }

    return hlen;
}

#ifdef IPV6
int decode_icmp_ipv6(
    struct sockaddr *response_addr,
    size_t response_addr_len,
    char *reply_buf,
    size_t reply_buf_len,
    unsigned short *id,
    unsigned short *seq,
    IP_HEADER_RESULT *ip_header_res)
{
    struct icmp6_hdr *icp;

    if (reply_buf_len < sizeof(struct icmp6_hdr)) {
        if (opt_verbose_on) {
            char buf[INET6_ADDRSTRLEN];
            getnameinfo(response_addr, response_addr_len, buf, INET6_ADDRSTRLEN, NULL, 0, NI_NUMERICHOST);
            printf("received packet too short for ICMPv6 (%d bytes from %s)\n", (int)reply_buf_len, buf);
        }
        return 0; /* too short */
    }

    icp = (struct icmp6_hdr *)reply_buf;

    if (icp->icmp6_type != ICMP6_ECHO_REPLY) {
        /* Handle other ICMPv6 packets */
        struct ip6_hdr *sent_ipv6;
        struct icmp6_hdr *sent_icmp;
        SEQMAP_VALUE *seqmap_value;
        char addr_ascii[INET6_ADDRSTRLEN];
        HOST_ENTRY *h;

        /* reply icmp packet (ICMPv6 header) followed by "sent packet" (IPv6 + ICMPv6 header) */
        if (reply_buf_len < ICMP_MINLEN + sizeof(struct ip6_hdr) + sizeof(struct icmp6_hdr)) {
            /* discard ICMPv6 message if we can't tell that it was caused by us (i.e. if the "sent packet" is not included). */
            return 0;
        }

        sent_ipv6 = (struct ip6_hdr *)(reply_buf + sizeof(struct icmp6_hdr));
        if (sent_ipv6->ip6_nxt != IPPROTO_ICMPV6) {
            /* discard ICMPv6 message if we can't tell that it was caused by
             * us, because the IPv6 header is not directly followed by an
             * ICMPv6 header
             */
            dbg_printf("invoking packet next header is %d\n", sent_ipv6->ip6_nxt);
            return 0;
        }
        sent_icmp = (struct icmp6_hdr *)(reply_buf + sizeof(struct icmp6_hdr) + sizeof(struct ip6_hdr));

        if (sent_icmp->icmp6_type != ICMP6_ECHO_REQUEST || sent_icmp->icmp6_id != ident6) {
            /* not caused by us */
            return 0;
        }

        seqmap_value = seqmap_fetch(ntohs(sent_icmp->icmp6_seq), current_time_ns);
        if (seqmap_value == NULL) {
            return 0;
        }

        getnameinfo(response_addr, response_addr_len, addr_ascii, INET6_ADDRSTRLEN, NULL, 0, NI_NUMERICHOST);
        h = table[seqmap_value->host_nr];

        switch (icp->icmp6_type) {
        case ICMP6_DST_UNREACH:
            if (icp->icmp6_code > ICMP6_UNREACH_MAXCODE) {
                print_warning("ICMPv6 Destination Unreachable (Code %d) from %s for ICMPv6 Echo Request sent to %s",
                    icp->icmp6_code, addr_ascii, h->host);
            } else {
                print_warning("ICMPv6 Destination Unreachable (%s) from %s for ICMPv6 Echo Request sent to %s",
                    icmp6_unreach_str[icp->icmp6_code], addr_ascii, h->host);
            }
            print_warning("\n");
            num_othericmprcvd++;
            break;

        case ICMP6_PACKET_TOO_BIG:
            print_warning("ICMPv6 Packet Too Big from %s for ICMPv6 Echo Request sent to %s\n",
                addr_ascii, h->host);
            num_othericmprcvd++;
            break;

        case ICMP6_TIME_EXCEEDED:
            if (icp->icmp6_code > ICMP6_TIME_EXCEEDED_MAXCODE) {
                print_warning("ICMPv6 Time Exceeded (Code %d) from %s for ICMPv6 Echo Request sent to %s",
                    icp->icmp6_code, addr_ascii, h->host);
            } else {
                print_warning("ICMPv6 Time Exceeded (%s) from %s for ICMPv6 Echo Request sent to %s",
                    icmp6_time_exceeded_str[icp->icmp6_code], addr_ascii, h->host);
            }
            print_warning("\n");
            num_othericmprcvd++;
            break;

        case ICMP6_PARAM_PROB:
            if (icp->icmp6_code > ICMP6_PARAM_PROB_MAXCODE) {
                print_warning("ICMPv6 Parameter Problem (Code %d) from %s for ICMPv6 Echo Request sent to %s",
                    icp->icmp6_code, addr_ascii, h->host);
            } else {
                print_warning("ICMPv6 Parameter Problem (%s) from %s for ICMPv6 Echo Request sent to %s",
                    icmp6_param_prob_str[icp->icmp6_code], addr_ascii, h->host);
            }
            print_warning("\n");
            num_othericmprcvd++;
            break;

        default:
            print_warning("ICMPv6 Type %d Code %d from %s for ICMPv6 Echo Request sent to %s\n",
                icp->icmp6_type, icp->icmp6_code, addr_ascii, h->host);
            num_othericmprcvd++;
            break;
        }

        return 0;
    }

    *id = icp->icmp6_id;
    *seq = ntohs(icp->icmp6_seq);

    if (opt_print_srcaddr_on) {
        strncpy(ip_header_res->src_addr, "not supported", sizeof(ip_header_res->src_addr) - 1);
        ip_header_res->src_addr[sizeof(ip_header_res->src_addr) - 1] = '\0';
    }

    return 1;
}
#endif

int wait_for_reply(int64_t wait_time)
{
    int result;
    static char buffer[RECV_BUFSIZE];
    struct sockaddr_storage response_addr;
    int n, avg;
    HOST_ENTRY *h;
    int64_t this_reply;
    int this_count;
    int64_t recv_time = 0;
    SEQMAP_VALUE *seqmap_value;
    unsigned short id;
    unsigned short seq;
    IP_HEADER_RESULT ip_header_res = default_ip_header_result();

    /* Receive packet */
    result = receive_packet(wait_time, /* max. wait time, in ns */
        &recv_time, /* reply_timestamp */
        (struct sockaddr *)&response_addr, /* reply_src_addr */
        sizeof(response_addr), /* reply_src_addr_len */
        buffer, /* reply_buf */
        sizeof(buffer), /* reply_buf_len */
        &ip_header_res.tos, /* TOS resp. TC byte */
        &ip_header_res.ttl /* TTL resp. hop limit */
    );

    if (result <= 0) {
        return 0;
    }

    update_current_time();
    if (recv_time == 0)
        recv_time = current_time_ns;

    /* Process ICMP packet and retrieve id/seq */
    if (response_addr.ss_family == AF_INET) {
        int ip_hlen = decode_icmp_ipv4(
            (struct sockaddr *)&response_addr,
            sizeof(response_addr),
            buffer,
            sizeof(buffer),
            &id,
            &seq,
            &ip_header_res);
        if (ip_hlen < 0) {
            return 1;
        }
        if (id != ident4) {
            return 1; /* packet received, but not the one we are looking for! */
        }
        if (!using_sock_dgram4) {
            /* do not include IP header in returned size, to be consistent with ping(8) and also
             * with fping with IPv6 hosts */
            result -= ip_hlen;
        }
    }
#ifdef IPV6
    else if (response_addr.ss_family == AF_INET6) {
        if (!decode_icmp_ipv6(
                (struct sockaddr *)&response_addr,
                sizeof(response_addr),
                buffer,
                sizeof(buffer),
                &id,
                &seq,
                &ip_header_res)) {
            return 1;
        }
        if (id != ident6) {
            return 1; /* packet received, but not the one we are looking for! */
        }
    }
#endif
    else {
        return 1;
    }

    seqmap_value = seqmap_fetch(seq, current_time_ns);
    if (seqmap_value == NULL) {
        return 1;
    }

    /* find corresponding host_entry */
    n = seqmap_value->host_nr;
    h = table[n];
    this_count = seqmap_value->ping_count;
    this_reply = recv_time - seqmap_value->ping_ts;

    /* update stats that include invalid replies */
    h->num_recv_total++;
    num_pingreceived++;

    dbg_printf("received [%d] from %s\n", this_count, h->host);

    /* optionally require reply source equal to target address */
    if (opt_check_source_on && addr_cmp((struct sockaddr *)&response_addr, (struct sockaddr *)&h->saddr)) {
        dbg_printf("%s\n", "discarding reply from wrong source address");
        return 1;
    }

    /* discard duplicates */
    if (!opt_loop_on && !(opt_count_on && opt_quiet_on) && h->resp_times[this_count] >= 0) {
        if (!opt_per_recv_on) {
            fprintf(stderr, "%s : duplicate for [%d], %d bytes, %s ms",
                h->host, this_count, result, sprint_tm(this_reply));

            if (addr_cmp((struct sockaddr *)&response_addr, (struct sockaddr *)&h->saddr)) {
                char buf[INET6_ADDRSTRLEN];
                getnameinfo((struct sockaddr *)&response_addr, sizeof(response_addr), buf, INET6_ADDRSTRLEN, NULL, 0, NI_NUMERICHOST);
                fprintf(stderr, " [<- %s]", buf);
            }
            fprintf(stderr, "\n");
        }
        return 1;
    }

    /* discard reply if delay is larger than timeout
     * (see also: github #32) */
    if (this_reply > h->timeout) {
        return 1;
    }

    /* update stats */
    stats_add(h, this_count, 1, this_reply);
    // TODO: move to stats_add?
    if (!max_reply || this_reply > max_reply)
        max_reply = this_reply;
    if (!min_reply || this_reply < min_reply)
        min_reply = this_reply;
    sum_replies += this_reply;
    total_replies++;

    /* initialize timeout to initial timeout (without backoff) */
    h->timeout = opt_timeout;

    /* remove timeout event */
    struct event *timeout_event = host_get_timeout_event(h, this_count);
    if (timeout_event) {
        ev_remove(&event_queue_timeout, timeout_event);
    }

    /* print "is alive" */
    if (h->num_recv == 1) {
        num_alive++;
        if (opt_fast_reachable_on && num_alive >= opt_min_reachable)
            finish_requested = 1;

        if (opt_verbose_on || opt_alive_on) {
            printf("%s", h->host);

            if (opt_verbose_on)
                printf(" is alive");
        }
    }

    /* print received ping (unless --quiet) */
    if (opt_per_recv_on) {
        avg = h->total_time / h->num_recv;
        print_recv(h,
            recv_time,
            result,
            this_count,
            this_reply,
            avg);
    }

    if (opt_verbose_on || opt_alive_on || opt_per_recv_on) {
        if (addr_cmp((struct sockaddr *)&response_addr, (struct sockaddr *)&h->saddr)) {
            char buf[INET6_ADDRSTRLEN];
            getnameinfo((struct sockaddr *)&response_addr, sizeof(response_addr), buf, INET6_ADDRSTRLEN, NULL, 0, NI_NUMERICHOST);
            fprintf(stderr, " [<- %s]", buf);
        }
        if (opt_print_json_on) {
            print_recv_ext_json(&ip_header_res,
                recv_time,
                this_reply);
        }
        else {
            print_recv_ext(&ip_header_res,
                recv_time,
                this_reply);
        }
    }
    return 1;
}

/************************************************************

  Function: add_name

*************************************************************

  Inputs:  char* name

  Description:

  process input name for addition to target list
  name can turn into multiple targets via multiple interfaces (-m)
  or via NIS groups

************************************************************/

void add_name(char *name)
{
    struct addrinfo *res0, *res, hints;
    int ret_ga;
    char *printname;
    char namebuf[256];
    char addrbuf[256];

    /* getaddrinfo */
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_flags = AI_UNUSABLE;
    hints.ai_socktype = SOCK_RAW;
    hints.ai_family = hints_ai_family;
    if (hints_ai_family == AF_INET) {
        hints.ai_protocol = IPPROTO_ICMP;
    }
#ifdef IPV6
    else if (hints_ai_family == AF_INET6) {
        hints.ai_protocol = IPPROTO_ICMPV6;
    }
#endif
    else {
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = 0;
    }
    ret_ga = getaddrinfo(name, NULL, &hints, &res0);
    if (ret_ga) {
        if (!opt_quiet_on)
            print_warning("%s: %s\n", name, gai_strerror(ret_ga));
        num_noaddress++;

        // Handle JSON output for invalid hosts
        if (opt_print_json_on) {
            fprintf(stdout, "{\"warning\": {\"host\": \"%s\", \"message\": \"%s\"}}\n", name, gai_strerror(ret_ga));
            return;
        }

        return;
    }

    /* NOTE: we could/should loop with res on all addresses like this:
     * for (res = res0; res; res = res->ai_next) {
     * We don't do it yet, however, because is is an incompatible change
     * (need to implement a separate option for this)
     */
    for (res = res0; res; res = res->ai_next) {
        /* opt_name_on: addr -> name lookup requested) */
        if (opt_name_on || opt_rdns_on) {
            int do_rdns = opt_rdns_on ? 1 : 0;
            if (opt_name_on) {
                /* Was it a numerical address? Only then do a rdns-query */
                struct addrinfo *nres;
                hints.ai_flags = AI_NUMERICHOST;
                if (getaddrinfo(name, NULL, &hints, &nres) == 0) {
                    do_rdns = 1;
                    freeaddrinfo(nres);
                }
            }

            if (do_rdns && getnameinfo(res->ai_addr, res->ai_addrlen, namebuf, sizeof(namebuf) / sizeof(char), NULL, 0, 0) == 0) {
                printname = namebuf;
            }
            else {
                printname = name;
            }
        }
        else {
            printname = name;
        }

        /* opt_addr_on: name -> addr lookup requested */
        if (opt_addr_on) {
            int ret;
            ret = getnameinfo(res->ai_addr, res->ai_addrlen, addrbuf,
                sizeof(addrbuf) / sizeof(char), NULL, 0, NI_NUMERICHOST);
            if (ret) {
                if (!opt_quiet_on) {
                    print_warning("%s: can't forward-lookup address (%s)\n", name, gai_strerror(ret));
                }
                continue;
            }

            if (opt_name_on || opt_rdns_on) {
                char nameaddrbuf[512 + 3];
                snprintf(nameaddrbuf, sizeof(nameaddrbuf) / sizeof(char), "%s (%s)", printname, addrbuf);
                add_addr(name, nameaddrbuf, res->ai_addr, res->ai_addrlen);
            }
            else {
                add_addr(name, addrbuf, res->ai_addr, res->ai_addrlen);
            }
        }
        else {
            add_addr(name, printname, res->ai_addr, res->ai_addrlen);
        }

        if (!opt_multif_on) {
            break;
        }
    }

    freeaddrinfo(res0);
}

/************************************************************

  Function: add_addr

*************************************************************

  Description:

  add single address to list of hosts to be pinged

************************************************************/

void add_addr(char *name, char *host, struct sockaddr *ipaddr, socklen_t ipaddr_len)
{
    HOST_ENTRY *p;
    int n;
    int64_t *i;

    p = (HOST_ENTRY *)calloc(1, sizeof(HOST_ENTRY));
    if (!p)
        crash_and_burn("can't allocate HOST_ENTRY");

    p->name = strdup(name);
    p->host = strdup(host);
    memcpy(&p->saddr, ipaddr, ipaddr_len);
    p->saddr_len = ipaddr_len;
    p->timeout = opt_timeout;
    p->min_reply = 0;

    if (opt_print_netdata_on) {
        char *s = p->name;
        while (*s) {
            if (!isalnum(*s))
                *s = '_';
            s++;
        }
    }

    if (strlen(p->host) > max_hostname_len)
        max_hostname_len = strlen(p->host);

    /* array for response time results */
    if (!opt_loop_on) {
#if SIZE_MAX <= UINT_MAX
        if (trials > (SIZE_MAX / sizeof(int64_t)))
            crash_and_burn("resp_times array too large for memory");
#endif
        i = (int64_t *)malloc(trials * sizeof(int64_t));
        if (!i)
            crash_and_burn("can't allocate resp_times array");

        for (n = 0; n < trials; n++)
            i[n] = RESP_UNUSED;

        p->resp_times = i;
    }

    /* allocate event storage */
    p->event_storage_ping = (struct event *)calloc(event_storage_count, sizeof(struct event));
    if (!p->event_storage_ping) {
        errno_crash_and_burn("can't allocate event_storage_ping");
    }
    p->event_storage_timeout = (struct event *)calloc(event_storage_count, sizeof(struct event));
    if (!p->event_storage_timeout) {
        errno_crash_and_burn("can't allocate event_storage_timeout");
    }

    /* schedule first ping */
    host_add_ping_event(p, 0, current_time_ns);

    num_hosts++;
}

/************************************************************

  Function: crash_and_burn

*************************************************************

  Inputs:  char* message

  Description:

************************************************************/

void crash_and_burn(char *message)
{
    fprintf(stderr, "%s: %s\n", prog, message);
    exit(4);
}

/************************************************************

  Function: errno_crash_and_burn

*************************************************************

  Inputs:  char* message

  Description:

************************************************************/

void errno_crash_and_burn(char *message)
{
    fprintf(stderr, "%s: %s : %s\n", prog, message, strerror(errno));
    exit(4);
}

/************************************************************

  Function: print_warning

  Description: fprintf(stderr, ...), unless running with -q

*************************************************************/

void print_warning(char *format, ...)
{
    va_list args;
    if (!opt_quiet_on) {
        va_start(args, format);
        vfprintf(stderr, format, args);
        va_end(args);
    }
}



/************************************************************

  Function: addr_cmp

*************************************************************/
int addr_cmp(struct sockaddr *a, struct sockaddr *b)
{
    if (a->sa_family != b->sa_family) {
        return a->sa_family - b->sa_family;
    }
    else {
        if (a->sa_family == AF_INET) {
            return ((struct sockaddr_in *)a)->sin_addr.s_addr - ((struct sockaddr_in *)b)->sin_addr.s_addr;
        }
        else if (a->sa_family == AF_INET6) {
            return memcmp(&((struct sockaddr_in6 *)a)->sin6_addr,
                &((struct sockaddr_in6 *)b)->sin6_addr,
                sizeof(((struct sockaddr_in6 *)a)->sin6_addr));
        }
    }

    return 0;
}

void host_add_ping_event(HOST_ENTRY *h, int index, int64_t ev_time)
{
    struct event *event = &h->event_storage_ping[index % event_storage_count];
    event->host = h;
    event->ping_index = index;
    event->ev_time = ev_time;
    ev_enqueue(&event_queue_ping, event);

    dbg_printf("%s [%d]: add ping event in %.0f ms\n",
        event->host->host, index, (ev_time - current_time_ns) / 1e6);
}

void host_add_timeout_event(HOST_ENTRY *h, int index, int64_t ev_time)
{
    struct event *event = &h->event_storage_timeout[index % event_storage_count];
    event->host = h;
    event->ping_index = index;
    event->ev_time = ev_time;
    ev_enqueue(&event_queue_timeout, event);

    dbg_printf("%s [%d]: add timeout event in %.0f ms\n",
        event->host->host, index, (ev_time - current_time_ns) / 1e6);
}

struct event *host_get_timeout_event(HOST_ENTRY *h, int index)
{
    return &h->event_storage_timeout[index % event_storage_count];
}

/************************************************************

  Function: ev_enqueue

  Enqueue an event

  The queue is sorted by event->ev_time, so that queue->first always points to
  the earliest event.

  We start scanning the queue from the tail, because we assume
  that new events mostly get inserted with a event time higher
  than the others.

*************************************************************/
void ev_enqueue(struct event_queue *queue, struct event *event)
{
    struct event *i;
    struct event *i_prev;

    /* Empty list */
    if (queue->last == NULL) {
        event->ev_next = NULL;
        event->ev_prev = NULL;
        queue->first = event;
        queue->last = event;
        return;
    }

    /* Insert on tail? */
    if (event->ev_time - queue->last->ev_time >= 0) {
        event->ev_next = NULL;
        event->ev_prev = queue->last;
        queue->last->ev_next = event;
        queue->last = event;
        return;
    }

    /* Find insertion point */
    i = queue->last;
    while (1) {
        i_prev = i->ev_prev;
        if (i_prev == NULL || event->ev_time - i_prev->ev_time >= 0) {
            event->ev_prev = i_prev;
            event->ev_next = i;
            i->ev_prev = event;
            if (i_prev != NULL) {
                i_prev->ev_next = event;
            }
            else {
                queue->first = event;
            }
            return;
        }
        i = i_prev;
    }
}

/************************************************************

  Function: ev_dequeue

*************************************************************/
struct event *ev_dequeue(struct event_queue *queue)
{
    struct event *dequeued;

    if (queue->first == NULL) {
        return NULL;
    }
    dequeued = queue->first;
    ev_remove(queue, dequeued);

    return dequeued;
}

/************************************************************

  Function: ev_remove

*************************************************************/
void ev_remove(struct event_queue *queue, struct event *event)
{
    if (queue->first == event) {
        queue->first = event->ev_next;
    }
    if (queue->last == event) {
        queue->last = event->ev_prev;
    }
    if (event->ev_prev) {
        event->ev_prev->ev_next = event->ev_next;
    }
    if (event->ev_next) {
        event->ev_next->ev_prev = event->ev_prev;
    }
    event->ev_prev = NULL;
    event->ev_next = NULL;
}



/************************************************************

  Function: usage

*************************************************************

  Inputs:  int: 0 if output on request, 1 if output because of wrong argument

  Description:

************************************************************/

void usage(int is_error)
{
    FILE *out = is_error ? stderr : stdout;
    fprintf(out, "Usage: %s [options] [targets...]\n", prog);
    fprintf(out, "\n");
    fprintf(out, "Probing options:\n");
    fprintf(out, "   -4, --ipv4         only ping IPv4 addresses\n");
    fprintf(out, "   -6, --ipv6         only ping IPv6 addresses\n");
    fprintf(out, "   -b, --size=BYTES   amount of ping data to send, in bytes (default: %d)\n", DEFAULT_PING_DATA_SIZE);
    fprintf(out, "   -B, --backoff=N    set exponential backoff factor to N (default: 1.5)\n");
    fprintf(out, "   -c, --count=N      count mode: send N pings to each target and report stats\n");
    fprintf(out, "   -f, --file=FILE    read list of targets from a file ( - means stdin)\n");
    fprintf(out, "   -g, --generate     generate target list (only if no -f specified),\n");
    fprintf(out, "                      limited to at most %d targets\n", MAX_GENERATE);
    fprintf(out, "                      (give start and end IP in the target list, or a CIDR address)\n");
    fprintf(out, "                      (ex. %s -g 192.168.1.0 192.168.1.255 or %s -g 192.168.1.0/24)\n", prog, prog);
    fprintf(out, "   -H, --ttl=N        set the IP TTL value (Time To Live hops)\n");
    fprintf(out, "   -i, --interval=MSEC  interval between sending ping packets (default: %.0f ms)\n", opt_interval / 1e6);
#ifdef SO_BINDTODEVICE
    fprintf(out, "   -I, --iface=IFACE  bind to a particular interface\n");
#endif
#ifdef IP_PKTINFO
    fprintf(out, "       --oiface=IFACE  send pings via a specific outgoing interface (receive from any)\n");
#endif
#ifdef SO_MARK
    fprintf(out, "   -k, --fwmark=FWMARK set the routing mark\n");
#endif
    fprintf(out, "   -l, --loop         loop mode: send pings forever\n");
    fprintf(out, "   -m, --all          use all IPs of provided hostnames (e.g. IPv4 and IPv6), use with -A\n");
    fprintf(out, "   -M, --dontfrag     set the Don't Fragment flag\n");
    fprintf(out, "   -O, --tos=N        set the type of service (tos) flag on the ICMP packets\n");
    fprintf(out, "   -p, --period=MSEC  interval between ping packets to one target (in ms)\n");
    fprintf(out, "                      (in loop and count modes, default: %.0f ms)\n", opt_perhost_interval / 1e6);
    fprintf(out, "   -r, --retry=N      number of retries (default: %d)\n", DEFAULT_RETRY);
    fprintf(out, "   -R, --random       random packet data (to foil link data compression)\n");
    fprintf(out, "   -S, --src=IP       set source address\n");
    fprintf(out, "       --seqmap-timeout=MSEC sequence number mapping timeout (default: %.0f ms)\n", opt_seqmap_timeout / 1e6);
    fprintf(out, "   -t, --timeout=MSEC individual target initial timeout (default: %.0f ms,\n", opt_timeout / 1e6);
    fprintf(out, "                      except with -l/-c/-C, where it's the -p period up to 2000 ms)\n");
    fprintf(out, "       --check-source discard replies not from target address\n");
    fprintf(out, "       --icmp-timestamp use ICMP Timestamp instead of ICMP Echo\n");
    fprintf(out, "\n");
    fprintf(out, "Output options:\n");
    fprintf(out, "   -a, --alive        show targets that are alive\n");
    fprintf(out, "   -A, --addr         show targets by address\n");
    fprintf(out, "   -C, --vcount=N     same as -c, report results (not stats) in verbose format\n");
    fprintf(out, "   -d, --rdns         show targets by name (force reverse-DNS lookup)\n");
    fprintf(out, "   -D, --timestamp    print timestamp before each output line\n");
    fprintf(out, "       --timestamp-format=FORMAT  show timestamp in the given format (-D required): ctime|iso|rfc3339\n");
    fprintf(out, "   -e, --elapsed      show elapsed time on return packets\n");
    fprintf(out, "   -J, --json         output in JSON format (-c, -C, or -l required)\n");
    fprintf(out, "   -n, --name         show targets by name (reverse-DNS lookup for target IPs)\n");
    fprintf(out, "   -N, --netdata      output compatible for netdata (-l -Q are required)\n");
    fprintf(out, "   -o, --outage       show the accumulated outage time (lost packets * packet interval)\n");
    fprintf(out, "   -q, --quiet        quiet (don't show per-target/per-ping results)\n");
    fprintf(out, "   -Q, --squiet=SECS[,cumulative]  same as -q, but add interval summary every SECS seconds,\n");
    fprintf(out, "                                   with 'cumulative', print stats since beginning\n");
    fprintf(out, "   -s, --stats        print final stats\n");
    fprintf(out, "   -u, --unreach      show targets that are unreachable\n");
    fprintf(out, "   -v, --version      show version\n");
    fprintf(out, "   -x, --reachable=N  shows if >=N hosts are reachable or not\n");
    fprintf(out, "   -X, --fast-reachable=N exits true immediately when N hosts are found\n");
    fprintf(out, "       --print-tos    show received TOS value\n");
    fprintf(out, "       --print-ttl    show IP TTL value\n");
    fprintf(out, "       --print-srcaddr show used IP source address (IPv6 is currently not supported).\n");
    exit(is_error);
}
