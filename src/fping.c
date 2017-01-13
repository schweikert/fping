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
extern "C"
{
#endif /* __cplusplus */

#include "fping.h"
#include "options.h"

/* if compiling for Windows, use this separate set
  (too difficult to ifdef all the autoconf defines) */
#ifdef WIN32

/*** Windows includes ***/


#else

/*** autoconf includes ***/


#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <stdarg.h>

#include "config.h"
#include "seqmap.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#include <string.h>
#include <stddef.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>

#if HAVE_SYS_FILE_H
#include <sys/file.h>
#endif /* HAVE_SYS_FILE_H */

#ifdef IPV6
#include <netinet/icmp6.h>
#endif
#include <netinet/in_systm.h>

#include <netinet/ip.h>
#include <netinet/ip_icmp.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>

/* RS6000 hasn't getopt.h */
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif /* HAVE_GETOPT_H */

/* RS6000 has sys/select.h */
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif /* HAVE_SYS_SELECT_H */

#endif /* WIN32 */

/*** externals ***/

extern char *optarg;
extern int optind,opterr;
#ifndef h_errno
extern int h_errno;
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

/*** Constants ***/

#define EMAIL       "david@schweikert.ch"

/*** Ping packet defines ***/

#define MAX_IP_PACKET   65536   /* (theoretical) max IP packet size */
#define SIZE_IP_HDR     20
#ifndef IPV6
#define SIZE_ICMP_HDR   ICMP_MINLEN     /* from ip_icmp.h */
#else
#define SIZE_ICMP_HDR   sizeof(FPING_ICMPHDR)
#endif
#define MAX_PING_DATA   ( MAX_IP_PACKET - SIZE_IP_HDR - SIZE_ICMP_HDR )

/* sized so as to be like traditional ping */
#define DEFAULT_PING_DATA_SIZE  56

/* maxima and minima */
#define MAX_COUNT               10000
#define MIN_INTERVAL            1       /* in millisec */
#define MIN_PERHOST_INTERVAL    20      /* in millisec */
#define MIN_TIMEOUT             50      /* in millisec */
#define MAX_RETRY               20

/* response time array flags */
#define RESP_WAITING    -1
#define RESP_UNUSED     -2
#define RESP_ERROR      -3

/* debugging flags */
#if defined( DEBUG ) || defined( _DEBUG )
#define DBG_TRACE               1
#define DBG_SENT_TIMES          2
#define DBG_RANDOM_LOSE_FEW     4
#define DBG_RANDOM_LOSE_MANY    8
#define DBG_PRINT_PER_SYSTEM    16
#define DBG_REPORT_ALL_RTTS     32
#endif /* DEBUG || _DEBUG */

/* Long names for ICMP packet types */
#define ICMP_TYPE_STR_MAX 18
char *icmp_type_str[19] =
{
    "ICMP Echo Reply",          /* 0 */
    "",
    "",
    "ICMP Unreachable",         /* 3 */
    "ICMP Source Quench",       /* 4 */
    "ICMP Redirect",            /* 5 */
    "",
    "",
    "ICMP Echo",                /* 8 */
    "",
    "",
    "ICMP Time Exceeded",       /* 11 */
    "ICMP Parameter Problem",   /* 12 */
    "ICMP Timestamp Request",   /* 13 */
    "ICMP Timestamp Reply",     /* 14 */
    "ICMP Information Request", /* 15 */
    "ICMP Information Reply",   /* 16 */
    "ICMP Mask Request",        /* 17 */
    "ICMP Mask Reply"           /* 18 */
};

char *icmp_unreach_str[16] =
{
    "ICMP Network Unreachable",                                     /* 0 */
    "ICMP Host Unreachable",                                        /* 1 */
    "ICMP Protocol Unreachable",                                    /* 2 */
    "ICMP Port Unreachable",                                        /* 3 */
    "ICMP Unreachable (Fragmentation Needed)",                      /* 4 */
    "ICMP Unreachable (Source Route Failed)",                       /* 5 */
    "ICMP Unreachable (Destination Network Unknown)",               /* 6 */
    "ICMP Unreachable (Destination Host Unknown)",                  /* 7 */
    "ICMP Unreachable (Source Host Isolated)",                      /* 8 */
    "ICMP Unreachable (Communication with Network Prohibited)",     /* 9 */
    "ICMP Unreachable (Communication with Host Prohibited)",        /* 10 */
    "ICMP Unreachable (Network Unreachable For Type Of Service)",   /* 11 */
    "ICMP Unreachable (Host Unreachable For Type Of Service)",      /* 12 */
    "ICMP Unreachable (Communication Administratively Prohibited)", /* 13 */
    "ICMP Unreachable (Host Precedence Violation)",                 /* 14 */
    "ICMP Unreachable (Precedence cutoff in effect)"                /* 15 */
};

#define ICMP_UNREACH_MAXTYPE    15

/* entry used to keep track of each host we are pinging */

#define EV_TYPE_PING     1
#define EV_TYPE_TIMEOUT  2

typedef struct host_entry
{
     /* Each host can have an event attached: either the next time that a ping needs
      * to be sent, or the timeout, if the last ping was sent */
     struct host_entry    *ev_prev;           /* double linked list for the event-queue */
     struct host_entry    *ev_next;           /* double linked list for the event-queue */
     struct timeval       ev_time;            /* time, after which this event should happen */
     int                  ev_type;            /* event type */

     int                  i;                  /* index into array */
     char                 *name;              /* name as given by user */
     char                 *host;              /* text description of host */
     char                 *pad;               /* pad to align print names */
     struct sockaddr_storage saddr;             /* internet address */
     socklen_t            saddr_len;
     int                  timeout;            /* time to wait for response */
     unsigned char        running;            /* unset when through sending */
     unsigned char        waiting;            /* waiting for response */
     struct timeval       last_send_time;     /* time of last packet sent */
     int                  num_sent;           /* number of ping packets sent */
     int                  num_recv;           /* number of pings received (duplicates ignored) */
     int                  num_recv_total;     /* number of pings received, including duplicates */
     int                  max_reply;          /* longest response time */
     int                  min_reply;          /* shortest response time */
     int                  total_time;         /* sum of response times */
     /* _i -> splits (reset on every report interval) */
     int                  num_sent_i;         /* number of ping packets sent */
     int                  num_recv_i;         /* number of pings received */
     int                  max_reply_i;        /* longest response time */
     int                  min_reply_i;        /* shortest response time */
     int                  total_time_i;       /* sum of response times */
     int                  discard_next_recv_i;/* don't count next received reply for split reporting */
     int                  *resp_times;        /* individual response times */
#if defined( DEBUG ) || defined( _DEBUG )
     int                  *sent_times;        /* per-sent-ping timestamp */
#endif /* DEBUG || _DEBUG */
} HOST_ENTRY;

/*** globals ***/

HOST_ENTRY **table = NULL;  /* array of pointers to items in the list */

/* event queue (ev): This, together with the ev_next / ev_prev elements are used
 * to track the next event happening for each host. This can be either a new ping
 * that needs to be sent or a timeout */
HOST_ENTRY *ev_first;
HOST_ENTRY *ev_last;

char *prog;
int ident;                  /* our pid */
int s;                      /* socket */
unsigned int debugging = 0;

/* times get *100 because all times are calculated in 10 usec units, not ms */
unsigned int retry = DEFAULT_RETRY;
unsigned int timeout = DEFAULT_TIMEOUT * 100; 
unsigned int interval = DEFAULT_INTERVAL * 100;
unsigned int perhost_interval = DEFAULT_PERHOST_INTERVAL * 100;
float backoff = DEFAULT_BACKOFF_FACTOR;
unsigned int ping_data_size = DEFAULT_PING_DATA_SIZE;
unsigned int count = 1;
unsigned int trials;
unsigned int report_interval = 0;
unsigned int ttl = 0;
int src_addr_present = 0;
#ifndef IPV6
struct in_addr src_addr;
#else
struct in6_addr src_addr;
#endif

/* global stats */
long max_reply = 0;
long min_reply = 0;
int total_replies = 0;
double sum_replies = 0;
int max_hostname_len = 0;
int num_jobs = 0;                   /* number of hosts still to do */
int num_hosts;                      /* total number of hosts */
int num_alive = 0,                  /* total number alive */
    num_unreachable = 0,            /* total number unreachable */
    num_noaddress = 0;              /* total number of addresses not found */
int num_timeout = 0,                /* number of times select timed out */
    num_pingsent = 0,               /* total pings sent */
    num_pingreceived = 0,           /* total pings received */
    num_othericmprcvd = 0;          /* total non-echo-reply ICMP received */

struct timeval current_time;        /* current time (pseudo) */
struct timeval start_time; 
struct timeval end_time;
struct timeval last_send_time;      /* time last ping was sent */
struct timeval next_report_time;    /* time next -Q report is expected */
struct timezone tz;

/* switches */
int generate_flag = 0;              /* flag for IP list generation */
int verbose_flag, quiet_flag, stats_flag, unreachable_flag, alive_flag;
int elapsed_flag, version_flag, count_flag, loop_flag, netdata_flag;
int per_recv_flag, report_all_rtts_flag, name_flag, addr_flag, backoff_flag;
int multif_flag;
int outage_flag = 0;
int timestamp_flag = 0;
int random_data_flag = 0;
#if defined( DEBUG ) || defined( _DEBUG )
int randomly_lose_flag, sent_times_flag, trace_flag, print_per_system_flag;
int lose_factor;
#endif /* DEBUG || _DEBUG */

char *filename = NULL;              /* file containing hosts to ping */

/*** forward declarations ***/

void add_name( char *name );
void add_addr( char *name, char *host, struct sockaddr *ipaddr, socklen_t ipaddr_len);
char *na_cat( char *name, struct in_addr ipaddr );
void crash_and_burn( char *message );
void errno_crash_and_burn( char *message );
char *get_host_by_address( struct in_addr in );
int recvfrom_wto( int s, char *buf, int len, struct sockaddr *saddr, socklen_t *saddr_len, long timo );
void remove_job( HOST_ENTRY *h );
int send_ping( int s, HOST_ENTRY *h );
long timeval_diff( struct timeval *a, struct timeval *b );
void timeval_add(struct timeval *a, long t_10u);
void usage( int );
int wait_for_reply( long );
void print_per_system_stats( void );
void print_per_system_splits( void );
void print_netdata( void );
void print_global_stats( void );
void main_loop();
void finish();
int handle_random_icmp( FPING_ICMPHDR *p, struct sockaddr *addr, socklen_t addr_len);
char *sprint_tm( int t );
void ev_enqueue(HOST_ENTRY  *h);
HOST_ENTRY *ev_dequeue();
void ev_remove(HOST_ENTRY *h);
void add_cidr(char *);
void add_range(char *, char *);
void print_warning(char *fmt, ...);
int addr_cmp(struct sockaddr *a, struct sockaddr *b);

/*** function definitions ***/

/************************************************************

  Function: main

*************************************************************

  Inputs:  int argc, char** argv

  Description:
  
  Main program entry point

************************************************************/

int main( int argc, char **argv )
{
    int c, i, n;
    char *buf;
    uid_t uid;
    int tos = 0; 
    HOST_ENTRY *cursor;

    prog = argv[0];

    s = open_ping_socket(ping_data_size);

    if((uid = getuid())) {
        /* drop privileges */
        if(setuid( getuid() ) == -1)
            perror("cannot setuid");
    }

    ident = getpid() & 0xFFFF;
    verbose_flag = 1;
    backoff_flag = 1;
    opterr = 1;

    /* get command line options */

    while( ( c = getopt( argc, argv, "gedhlmnNqusaAvDRz:t:H:i:p:f:r:c:b:C:Q:B:S:I:T:O:M:o" ) ) != EOF )
    {
        switch( c )
        {
        case 'M':
#ifdef IP_MTU_DISCOVER
            {
                int val = IP_PMTUDISC_DO;
                if (setsockopt(s, IPPROTO_IP, IP_MTU_DISCOVER, &val, sizeof(val))) {
                    perror("setsockopt IP_MTU_DISCOVER");
                }
            }
#else
            fprintf(stderr, "-M option not supported on this platform\n");
            exit(1);
#endif
            break;

        case 't':
            if( !( timeout = ( unsigned int )atoi( optarg ) * 100 ) )
                usage(1);

            break;
        
        case 'r':
            if (!sscanf(optarg,"%u",&retry))
                usage(1);
            break;
        
        case 'i':
            if (!sscanf(optarg,"%u",&interval))
                usage(1);
            interval *= 100;
            break;

        case 'p':
            if( !( perhost_interval = ( unsigned int )atoi( optarg ) * 100 ) )
                usage(1);

            break;

        case 'c':
            if( !( count = ( unsigned int )atoi( optarg ) ) )
                usage(1);
            
            count_flag = 1;
            break;
        
        case 'C':
            if( !( count = ( unsigned int )atoi( optarg ) ) )
                usage(1);
            
            count_flag = 1;
            report_all_rtts_flag = 1;
            break;

        case 'b':
            if (!sscanf(optarg,"%i",&ping_data_size))
                usage(1);
            
            break;

        case 'h':
            usage(0);
            break;

        case 'q':
            verbose_flag = 0;
            quiet_flag = 1;
            break;

        case 'Q':
            verbose_flag = 0;
            quiet_flag = 1;
            if( !( report_interval = ( unsigned int )atoi( optarg ) * 100000 ) )
                usage(1);
            
            break;

        case 'e':
            elapsed_flag = 1;
            break;

        case 'm':
            multif_flag = 1;
            break;

        case 'N':
            netdata_flag = 1;
            break;

        case 'd': 
        case 'n':
            name_flag = 1;
            break;

        case 'A':
            addr_flag = 1;
            break;

        case 'B':
            if( !( backoff = atof( optarg ) ) )
                usage(1);
            
            break;

        case 's':
            stats_flag = 1;
            break;

        case 'D':
            timestamp_flag = 1;
            break;

        case 'R':
            random_data_flag = 1;
            break;

        case 'l':
            loop_flag = 1;
            backoff_flag = 0;
            break;

        case 'u':
            unreachable_flag = 1;
            break;

        case 'a':
            alive_flag = 1;
            break;

        case 'H':  
            if( !( ttl = ( u_int )atoi( optarg ) ))
                usage(1);
            break;  

#if defined( DEBUG ) || defined( _DEBUG )
        case 'z':
            if( ! ( debugging = ( unsigned int )atoi( optarg ) ) )
                usage(1);
            
            break;
#endif /* DEBUG || _DEBUG */

        case 'v':
            printf( "%s: Version %s\n", argv[0], VERSION);
            printf( "%s: comments to %s\n", argv[0], EMAIL );
            exit( 0 );

        case 'f': 
            filename = optarg;
            break;

        case 'g':
            /* use IP list generation */
            /* mutually exclusive with using file input or command line targets */
            generate_flag = 1;
            break;

        case 'S':
#ifndef IPV6
            if( ! inet_pton( AF_INET, optarg, &src_addr ) )
#else
            if( ! inet_pton( AF_INET6, optarg, &src_addr ) )
#endif
                usage(1);
            src_addr_present = 1;
            break;

        case 'I':
#ifdef SO_BINDTODEVICE
            if (setsockopt(s, SOL_SOCKET, SO_BINDTODEVICE, optarg, strlen(optarg))) {
                perror("binding to specific interface (SO_BINTODEVICE)");
            }
#else
            printf( "%s: cant bind to a particular net interface since SO_BINDTODEVICE is not supported on your os.\n", argv[0] );
            exit(3);;
#endif
            break;

        case 'T':
            /* This option is ignored for compatibility reasons ("select timeout" is not meaningful anymore) */
            break;

        case 'O':
            if (sscanf(optarg,"%i",&tos)){
                if ( setsockopt(s, IPPROTO_IP, IP_TOS, &tos, sizeof(tos))) {
                    perror("setting type of service octet IP_TOS");
                }
            }
            else {
                usage(1);
            }
            break;

        case 'o':
            outage_flag = 1;
            break;

        default:
            fprintf(stderr, "see 'fping -h' for usage information\n");
            exit(1);
            break;

        }/* SWITCH */
    }/* WHILE */

    /* validate various option settings */

    if (ttl > 255) {
        fprintf(stderr, "ttl %u out of range\n", ttl);  
        exit(1);
    }  

    if( unreachable_flag && alive_flag )
    {
        fprintf( stderr, "%s: specify only one of a, u\n", argv[0] );
        exit(1);

    }/* IF */

    if( count_flag && loop_flag )
    {
        fprintf( stderr, "%s: specify only one of c, l\n", argv[0] );
        exit(1);
    
    }/* IF */
    
    if( ( interval < MIN_INTERVAL * 100 ||
            perhost_interval < MIN_PERHOST_INTERVAL * 100 || 
            retry > MAX_RETRY || 
            timeout < MIN_TIMEOUT * 100 ) 
        && getuid() )
    {
        fprintf( stderr, "%s: these options are too risky for mere mortals.\n", prog );
        fprintf( stderr, "%s: You need i >= %u, p >= %u, r < %u, and t >= %u\n",
            prog, MIN_INTERVAL, MIN_PERHOST_INTERVAL, MAX_RETRY, MIN_TIMEOUT );
        exit(1);
    }/* IF */
    
    if( ping_data_size > MAX_PING_DATA )
    {
        fprintf( stderr, "%s: data size %u not valid, must be lower than %u\n",
            prog, ping_data_size, (unsigned int) MAX_PING_DATA );
        exit(1);
    
    }/* IF */
    
    if( ( backoff > MAX_BACKOFF_FACTOR ) || ( backoff < MIN_BACKOFF_FACTOR ) )
    {
        fprintf( stderr, "%s: backoff factor %.1f not valid, must be between %.1f and %.1f\n",
            prog, backoff, MIN_BACKOFF_FACTOR, MAX_BACKOFF_FACTOR );
        exit(1);
    
    }/* IF */

    if( count > MAX_COUNT )
    {
        fprintf( stderr, "%s: count %u not valid, must be less than %u\n",
            prog, count, MAX_COUNT );
        exit(1);
    
    }/* IF */
    
    if( alive_flag || unreachable_flag )
        verbose_flag = 0;
    
    if( count_flag )
    {
        if( verbose_flag )
            per_recv_flag = 1;
    
        alive_flag = unreachable_flag = verbose_flag = 0;

    }/* IF */
    
    if( loop_flag )
    {
        if( !report_interval )
            per_recv_flag = 1;
        
        alive_flag = unreachable_flag = verbose_flag = 0;

    }/* IF */
    

    trials = ( count > retry + 1 ) ? count : retry + 1;

#if defined( DEBUG ) || defined( _DEBUG )
    if( debugging & DBG_TRACE )
        trace_flag = 1;
    
    if( ( debugging & DBG_SENT_TIMES ) && !loop_flag )
        sent_times_flag = 1;

    if( debugging & DBG_RANDOM_LOSE_FEW )
    {
        randomly_lose_flag = 1;
        lose_factor = 1;     /* ie, 1/4 */
    
    }/* IF */
    
    if( debugging & DBG_RANDOM_LOSE_MANY )
    {
        randomly_lose_flag = 1;
        lose_factor = 5;     /* ie, 3/4 */
    
    }/* IF */
  
    if( debugging & DBG_PRINT_PER_SYSTEM )
        print_per_system_flag = 1;

    if( ( debugging & DBG_REPORT_ALL_RTTS ) && !loop_flag )
        report_all_rtts_flag = 1;

    if( trace_flag )
    {
        fprintf( stderr, "%s:\n  count: %u, retry: %u, interval: %u\n",
            prog, count, retry, interval / 10 );
        fprintf( stderr, "  perhost_interval: %u, timeout: %u\n",
            perhost_interval / 10, timeout / 10 );
        fprintf( stderr, "  ping_data_size = %u, trials = %u\n",
            ping_data_size, trials );
        
        if( verbose_flag ) fprintf( stderr, "  verbose_flag set\n" );
        if( multif_flag ) fprintf( stderr, "  multif_flag set\n" );
        if( name_flag ) fprintf( stderr, "  name_flag set\n" );
        if( addr_flag ) fprintf( stderr, "  addr_flag set\n" );
        if( stats_flag ) fprintf( stderr, "  stats_flag set\n" );
        if( unreachable_flag ) fprintf( stderr, "  unreachable_flag set\n" );
        if( alive_flag ) fprintf( stderr, "  alive_flag set\n" );
        if( elapsed_flag ) fprintf( stderr, "  elapsed_flag set\n" );
        if( version_flag ) fprintf( stderr, "  version_flag set\n" );
        if( count_flag ) fprintf( stderr, "  count_flag set\n" );
        if( loop_flag ) fprintf( stderr, "  loop_flag set\n" );
        if( backoff_flag ) fprintf( stderr, "  backoff_flag set\n" );
        if( per_recv_flag ) fprintf( stderr, "  per_recv_flag set\n" );
        if( report_all_rtts_flag ) fprintf( stderr, "  report_all_rtts_flag set\n" );
        if( randomly_lose_flag ) fprintf( stderr, "  randomly_lose_flag set\n" );
        if( sent_times_flag ) fprintf( stderr, "  sent_times_flag set\n" );
        if( print_per_system_flag ) fprintf( stderr, "  print_per_system_flag set\n" );
        if( outage_flag ) fprintf( stderr, "  outage_flag set\n" );
        if( netdata_flag ) fprintf( stderr, "  netdata_flag set\n" );

    }/* IF */
#endif /* DEBUG || _DEBUG */

    /* set the TTL, if the -H option was set (otherwise ttl will be = 0) */
    if(ttl > 0) {
        if (setsockopt(s, IPPROTO_IP, IP_TTL,  &ttl, sizeof(ttl))) {
            perror("setting time to live");
        }
    }

#if HAVE_SO_TIMESTAMP 
    {
        int opt = 1;
        if (setsockopt(s, SOL_SOCKET, SO_TIMESTAMP,  &opt, sizeof(opt))) {
            perror("setting SO_TIMESTAMP option");
        }
    }
#endif

    /* handle host names supplied on command line or in a file */
    /* if the generate_flag is on, then generate the IP list */

    argv = &argv[optind];
    argc -= optind;

    /* cover allowable conditions */

    /* file and generate are mutually exclusive */
    /* file and command line are mutually exclusive */
    /* generate requires command line parameters beyond the switches */
    if( ( *argv && filename ) || ( filename && generate_flag ) || ( generate_flag && !*argv ) )
        usage(1);

    /* if no conditions are specified, then assume input from stdin */
    if( !*argv && !filename && !generate_flag )
        filename = "-";
    
    if( *argv && !generate_flag )
    {
        while( *argv )
        {
            add_name( *argv );
            ++argv;

        }/* WHILE */
    }/* IF */
    else if( filename )
    {
        FILE *ping_file;
        char line[132];
        char host[132];
        
        if( strcmp( filename, "-" ) == 0 )
            ping_file = fdopen( 0, "r" );
        else
            ping_file = fopen( filename, "r" );

        if( !ping_file )
            errno_crash_and_burn( "fopen" );


        while( fgets( line, sizeof(line), ping_file ) )
        {
            if( sscanf( line, "%s", host ) != 1 )
                continue;
            
            if( ( !*host ) || ( host[0] == '#' ) )  /* magic to avoid comments */
                continue;
            
            add_name(host);
        }/* WHILE */
        
        fclose( ping_file );
    }
    else if( *argv && generate_flag ) {
	if(argc == 1) {
	    /* one target: we expect a cidr range (n.n.n.n/m) */
	    add_cidr(argv[0]);
	}
	else if(argc == 2) {
	    add_range(argv[0], argv[1]);
	}
	else {
	    usage(1);
	}
    }
    else {
        usage(1);
    }
    
    if(!num_hosts) {
        exit(num_noaddress ? 2 : 1);
    }

    if(src_addr_present) {
        socket_set_src_addr(s, src_addr);
    }

    /* allocate array to hold outstanding ping requests */

    table = ( HOST_ENTRY** )malloc( sizeof( HOST_ENTRY* ) * num_hosts );
    if( !table )
        crash_and_burn( "Can't malloc array of hosts" );

    cursor = ev_first;

    for( num_jobs = 0; num_jobs < num_hosts; num_jobs++ )
    {
        table[num_jobs] = cursor;
        cursor->i = num_jobs;
        
        /* as long as we're here, put this in so names print out nicely */
        if( count_flag || loop_flag )
        {
            n = max_hostname_len - strlen( cursor->host );
            buf = ( char* ) malloc( n + 1 );
            if( !buf )
                crash_and_burn( "can't malloc host pad" );
            
            for ( i = 0; i < n; i++ )
                buf[i] = ' ';

            buf[n] = '\0';
            cursor->pad = buf;

        }/* IF */
        
        cursor=cursor->ev_next;

    }/* FOR */

    init_ping_buffer(ping_data_size);

    signal( SIGINT, finish );
    
    gettimeofday( &start_time, &tz );
    current_time = start_time;

    if( report_interval ) {
        next_report_time = start_time;
        timeval_add(&next_report_time, report_interval);
    }

    last_send_time.tv_sec = current_time.tv_sec - 10000;

#if defined( DEBUG ) || defined( _DEBUG )
    if( randomly_lose_flag ) 
        srandom( start_time.tv_usec );
#endif /* DEBUG || _DEBUG */

    seqmap_init();

    /* main loop */
    main_loop();

    finish();

    return 0;
} /* main() */

void add_cidr(char *addr)
{
    char *addr_end;
    char *mask_str; 
    unsigned long mask;
    unsigned long bitmask;
    int ret;
    struct addrinfo addr_hints;
    struct addrinfo *addr_res;
    unsigned long net_addr;
    unsigned long net_last;

    /* Split address from mask */
    addr_end = strchr(addr, '/');
    if(addr_end == NULL) {
        usage(1);
    }
    *addr_end = '\0';
    mask_str = addr_end + 1;
    mask = atoi(mask_str);

    /* parse address (IPv4 only) */
    memset(&addr_hints, 0, sizeof(struct addrinfo));
    addr_hints.ai_family = AF_UNSPEC;
    addr_hints.ai_flags = AI_NUMERICHOST;
    ret = getaddrinfo(addr, NULL, &addr_hints, &addr_res);
    if(ret) {
        fprintf(stderr, "Error: can't parse address %s: %s\n", addr, gai_strerror(ret));
        exit(1);
    }
    if(addr_res->ai_family != AF_INET) {
        fprintf(stderr, "Error: -g works only with IPv4 addresses\n");
        exit(1);
    }
    net_addr = ntohl(((struct sockaddr_in *) addr_res->ai_addr)->sin_addr.s_addr);

    /* check mask */
    if(mask < 1 || mask > 32) {
        fprintf(stderr, "Error: netmask must be between 1 and 32 (is: %s)\n", mask_str);
        exit(1);
    }

    /* convert mask integer from 1 to 32 to a bitmask */
    bitmask = ((unsigned long) 0xFFFFFFFF) << (32-mask);

    /* calculate network range */
    net_addr &= bitmask;
    net_last = net_addr + ((unsigned long) 0x1 << (32-mask)) - 1;

    /* exclude network and broadcast address for regular prefixes */
    if (mask < 31) {
        net_last--;
        net_addr++;
    }

    /* add all hosts in that network (net_addr and net_last inclusive) */
    for(; net_addr <= net_last; net_addr++) {
        struct in_addr in_addr_tmp;
        char buffer[20];
        in_addr_tmp.s_addr = htonl(net_addr);
        inet_ntop(AF_INET, &in_addr_tmp, buffer, sizeof(buffer));
        add_name(buffer);
    }


    freeaddrinfo(addr_res);
}

void add_range(char *start, char *end)
{
    struct addrinfo addr_hints;
    struct addrinfo *addr_res;
    unsigned long start_long;
    unsigned long end_long;
    int ret;

    /* parse start address (IPv4 only) */
    memset(&addr_hints, 0, sizeof(struct addrinfo));
    addr_hints.ai_family = AF_UNSPEC;
    addr_hints.ai_flags = AI_NUMERICHOST;
    ret = getaddrinfo(start, NULL, &addr_hints, &addr_res);
    if(ret) {
        fprintf(stderr, "Error: can't parse address %s: %s\n", start, gai_strerror(ret));
        exit(1);
    }
    if(addr_res->ai_family != AF_INET) {
        fprintf(stderr, "Error: -g works only with IPv4 addresses\n");
        exit(1);
    }
    start_long = ntohl(((struct sockaddr_in *) addr_res->ai_addr)->sin_addr.s_addr);

    /* parse end address (IPv4 only) */
    memset(&addr_hints, 0, sizeof(struct addrinfo));
    addr_hints.ai_family = AF_UNSPEC;
    addr_hints.ai_flags = AI_NUMERICHOST;
    ret = getaddrinfo(end, NULL, &addr_hints, &addr_res);
    if(ret) {
        fprintf(stderr, "Error: can't parse address %s: %s\n", end, gai_strerror(ret));
        exit(1);
    }
    if(addr_res->ai_family != AF_INET) {
        fprintf(stderr, "Error: -g works only with IPv4 addresses\n");
        exit(1);
    }
    end_long = ntohl(((struct sockaddr_in *) addr_res->ai_addr)->sin_addr.s_addr);

    /* generate */
    while(start_long <= end_long) {
        struct in_addr in_addr_tmp;
        char buffer[20];
        in_addr_tmp.s_addr = htonl(start_long);
        inet_ntop(AF_INET, &in_addr_tmp, buffer, sizeof(buffer));
        add_name(buffer);
        start_long++;
    }
}


void main_loop()
{
    long lt;
    long wait_time;
    long wait_time_next_report;
    HOST_ENTRY *h;

    while(ev_first) {
        /* Any event that can be processed now ? */
        if(ev_first->ev_time.tv_sec < current_time.tv_sec ||
            (ev_first->ev_time.tv_sec == current_time.tv_sec &&
             ev_first->ev_time.tv_usec < current_time.tv_usec))
        {
            /* Event type: ping */
            if(ev_first->ev_type == EV_TYPE_PING) {
                /* Make sure that we don't ping more than once every "interval" */
                lt = timeval_diff( &current_time, &last_send_time );
                if(lt < interval) goto wait_for_reply;

                /* Dequeue the event */
                h = ev_dequeue();

                /* Send the ping */
		/*printf("Sending ping after %d ms\n", lt/100); */
                send_ping(s, h);

                /* Check what needs to be done next */
                if(!loop_flag && !count_flag) {
                    /* Normal mode: schedule retry */
                    if(h->waiting < retry + 1) {
                        h->ev_type = EV_TYPE_PING;
                        h->ev_time.tv_sec = last_send_time.tv_sec;
                        h->ev_time.tv_usec = last_send_time.tv_usec;
                        timeval_add(&h->ev_time, h->timeout);
                        ev_enqueue(h);

                        if(backoff_flag) {
                            h->timeout *= backoff;
                        }
                    }
                    /* Normal mode: schedule timeout for last retry */
                    else {
                        h->ev_type = EV_TYPE_TIMEOUT;
                        h->ev_time.tv_sec = last_send_time.tv_sec;
                        h->ev_time.tv_usec = last_send_time.tv_usec;
                        timeval_add(&h->ev_time, h->timeout);
                        ev_enqueue(h);
                    }
                }
                /* Loop and count mode: schedule next ping */
                else if(loop_flag || (count_flag && h->num_sent < count))
                {
                    h->ev_type = EV_TYPE_PING;
                    h->ev_time.tv_sec = last_send_time.tv_sec;
                    h->ev_time.tv_usec = last_send_time.tv_usec;
                    timeval_add(&h->ev_time, perhost_interval);
                    ev_enqueue(h);
                }
                /* Count mode: schedule timeout after last ping */
                else if(count_flag && h->num_sent >= count) {
                    h->ev_type = EV_TYPE_TIMEOUT;
                    h->ev_time.tv_sec = last_send_time.tv_sec;
                    h->ev_time.tv_usec = last_send_time.tv_usec;
                    timeval_add(&h->ev_time, h->timeout);
                    ev_enqueue(h);
                }
            }
            /* Event type: timeout */
            else if(ev_first->ev_type == EV_TYPE_TIMEOUT) {
                num_timeout++;
                remove_job(ev_first);
            }
        }

        wait_for_reply:

        /* When can we expect the next event? */
        if(ev_first) {
            if(ev_first->ev_time.tv_sec == 0) {
		wait_time = 0;
	    }
	    else {
                wait_time = timeval_diff(&ev_first->ev_time, &current_time);
		if(wait_time < 0) wait_time = 0;
	    }
	    if(ev_first->ev_type == EV_TYPE_PING) {
		/* make sure that we wait enough, so that the inter-ping delay is
		 * bigger than 'interval' */
                if(wait_time < interval) {
		    lt = timeval_diff(&current_time, &last_send_time);
		    if(lt < interval) {
			wait_time = interval-lt;
		    }
		    else {
			wait_time = 0;
		    }
		}
            }

#if defined( DEBUG ) || defined( _DEBUG )
            if( trace_flag ) {
                fprintf(stderr, "next event in %d ms (%s)\n", wait_time / 100, ev_first->host);
            }
#endif
        }
        else {
            wait_time = interval;
        }

        /* Make sure we don't wait too long, in case a report is expected */
        if( report_interval && ( loop_flag || count_flag ) ) {
               wait_time_next_report = timeval_diff ( &next_report_time, &current_time );
               if(wait_time_next_report < wait_time) {
                    wait_time = wait_time_next_report;
                    if(wait_time < 0) { wait_time = 0; }
               }
        }

        /* Receive replies */
        /* (this is what sleeps during each loop iteration) */
        if(wait_for_reply(wait_time)) {
            while(wait_for_reply(0)); /* process other replies in the queue */
        }

        gettimeofday( &current_time, &tz );

        /* Print report */
        if( report_interval && ( loop_flag || count_flag ) &&
            ( timeval_diff ( &current_time, &next_report_time ) >= 0 ) )
        {
            if(netdata_flag)
                print_netdata();
            else
                print_per_system_splits();

            while(timeval_diff ( &current_time, &next_report_time ) >= 0 )
                timeval_add(&next_report_time, report_interval);
        }
    }
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

    gettimeofday( &end_time, &tz );

    /* tot up unreachables */
    for( i = 0; i < num_hosts; i++ )
    {
        h = table[i];

        if( !h->num_recv )
        {
            num_unreachable++;

            if( verbose_flag || unreachable_flag )
            {
                printf( "%s", h->host );

                if( verbose_flag ) 
                    printf( " is unreachable" );
                
                printf( "\n" );
            
            }/* IF */
        }/* IF */
    }/* FOR */

    if( count_flag || loop_flag )
        print_per_system_stats();
#if defined( DEBUG ) || defined( _DEBUG )
    else if( print_per_system_flag )
        print_per_system_stats();
#endif /* DEBUG || _DEBUG */

    if( stats_flag )
        print_global_stats();

    if( num_noaddress )
        exit( 2 );
    else if( num_alive != num_hosts )
        exit( 1 ); 
    
    exit(0);

} /* finish() */


/************************************************************

  Function: print_per_system_stats

*************************************************************

  Inputs:  void (none)

  Description:
  

************************************************************/

void print_per_system_stats( void )
{
    int i, j, avg, outage_ms;
    HOST_ENTRY *h;
    int resp;

    fflush( stdout );

    if( verbose_flag || per_recv_flag )
        fprintf( stderr, "\n" );

    for( i = 0; i < num_hosts; i++ )
    {
        h = table[i];
        fprintf( stderr, "%s%s :", h->host, h->pad );

        if( report_all_rtts_flag ) {
            for( j = 0; j < h->num_sent; j++ ) {
                if( ( resp = h->resp_times[j] ) >= 0 )
                    fprintf( stderr, " %d.%02d", resp / 100, resp % 100 );
                else
                    fprintf( stderr, " -" );
            }
          
            fprintf( stderr, "\n" );
        }
        else {
            if( h->num_recv <= h->num_sent )
            {
                fprintf( stderr, " xmt/rcv/%%loss = %d/%d/%d%%",
                    h->num_sent, h->num_recv, h->num_sent > 0 ?
                    ( ( h->num_sent - h->num_recv ) * 100 ) / h->num_sent : 0 );

                if (outage_flag) {
                    /* Time outage total */
                    outage_ms = (h->num_sent - h->num_recv) * perhost_interval/100;
                    fprintf( stderr, ", outage(ms) = %d", outage_ms );
                }

            }
            else {
                fprintf( stderr, " xmt/rcv/%%return = %d/%d/%d%%",
                    h->num_sent, h->num_recv,
                    ( ( h->num_recv * 100 ) / h->num_sent ) );
          
            }
          
            if( h->num_recv ) {
                avg = h->total_time / h->num_recv;
                fprintf( stderr, ", min/avg/max = %s", sprint_tm( h->min_reply ) );
                fprintf( stderr, "/%s", sprint_tm( avg ) );
                fprintf( stderr, "/%s", sprint_tm( h->max_reply ) );
          
            }
          
            fprintf(stderr, "\n");
        }

#if defined( DEBUG ) || defined( _DEBUG )
        if( sent_times_flag )
        {
            for( j = 0; j < h->num_sent; j++ )
            {
                if( ( resp = h->sent_times[j] ) >= 0 )
                    fprintf( stderr, " %s", sprint_tm( resp ) );
                else
                    fprintf( stderr, " -" );
              
                fprintf( stderr, "\n" );

            }
        }
#endif
    }
}


/************************************************************

  Function: print_netdata

*************************************************************

  Inputs:  void (none)

  Description:


************************************************************/

void print_netdata( void )
{
    static int sent_charts = 0;

    int i, avg;
    HOST_ENTRY *h;

    fflush( stdout );

    for( i = 0; i < num_hosts; i++ )
    {
        h = table[i];

        if(!sent_charts) {
            printf("CHART fping.%s_packets '' 'FPing Packets for host %s' packets '%s' fping.packets line 110020 %d\n", h->name, h->host, h->name, report_interval / 100000);
            printf("DIMENSION xmt sent absolute 1 1\n");
            printf("DIMENSION rcv received absolute 1 1\n");
        }

        printf("BEGIN fping.%s_packets\n", h->name);
        printf("SET xmt = %d\n", h->num_sent_i);
        printf("SET rcv = %d\n", h->num_recv_i);
        printf("END\n");

        if(!sent_charts) {
            printf("CHART fping.%s_quality '' 'FPing Quality for host %s' percentage '%s' fping.quality area 110010 %d\n", h->name, h->host, h->name, report_interval / 100000);
            printf("DIMENSION returned '' absolute 1 1\n");
            /* printf("DIMENSION lost '' absolute 1 1\n"); */
        }

        /* if we just sent the probe and didn't receive a reply, we shouldn't count it */
        h->discard_next_recv_i=0;
        if( h->waiting && timeval_diff(&current_time, &h->last_send_time) < h->timeout) {
            if(h->num_sent_i) {
                h->num_sent_i--;
                h->discard_next_recv_i=1;
            }
        }


        printf("BEGIN fping.%s_quality\n", h->name);
/*
        if( h->num_recv_i <= h->num_sent_i )
            printf("SET lost = %d\n", h->num_sent_i > 0 ? ( ( h->num_sent_i - h->num_recv_i ) * 100 ) / h->num_sent_i : 0 );
        else
            printf("SET lost = 0\n");
*/

        printf("SET returned = %d\n", h->num_sent_i > 0 ? ( ( h->num_recv_i * 100 ) / h->num_sent_i ) : 0 );
        printf("END\n");

        if(!sent_charts) {
            printf("CHART fping.%s_latency '' 'FPing Latency for host %s' ms '%s' fping.latency area 110000 %d\n", h->name, h->host, h->name, report_interval / 100000);
            printf("DIMENSION min minimum absolute 10 1000\n");
            printf("DIMENSION max maximum absolute 10 1000\n");
            printf("DIMENSION avg average absolute 10 1000\n");
        }

        printf("BEGIN fping.%s_latency\n", h->name);
        if( h->num_recv_i )
        {
            avg = h->total_time_i / h->num_recv_i;
            printf("SET min = %d\n", h->min_reply_i);
            printf("SET avg = %d\n", avg);
            printf("SET max = %d\n", h->max_reply_i);

        }/* IF */
        printf("END\n");

        h->num_sent_i = h->num_recv_i = h->max_reply_i =
        h->min_reply_i = h->total_time_i = 0;

    }/* FOR */

    sent_charts = 1;
} /* print_per_system_splits() */

/************************************************************

  Function: print_per_system_splits

*************************************************************

  Inputs:  void (none)

  Description:
  

************************************************************/

void print_per_system_splits( void )
{
    int i, avg, outage_ms_i;
    HOST_ENTRY *h;
    struct tm *curr_tm;

    fflush( stdout );

    if( verbose_flag || per_recv_flag )
        fprintf( stderr, "\n" );

    gettimeofday( &current_time, &tz );
    curr_tm = localtime( ( time_t* )&current_time.tv_sec );
    fprintf( stderr, "[%2.2d:%2.2d:%2.2d]\n", curr_tm->tm_hour,
        curr_tm->tm_min, curr_tm->tm_sec );

    for( i = 0; i < num_hosts; i++ ) {
        h = table[i];
        fprintf( stderr, "%s%s :", h->host, h->pad );

        /* if we just sent the probe and didn't receive a reply, we shouldn't count it */
        h->discard_next_recv_i=0;
        if( h->waiting && timeval_diff(&current_time, &h->last_send_time) < h->timeout) {
            if(h->num_sent_i) {
                h->num_sent_i--;
                h->discard_next_recv_i=1;
            }
        }

        if( h->num_recv_i <= h->num_sent_i ) {
            fprintf( stderr, " xmt/rcv/%%loss = %d/%d/%d%%",
                h->num_sent_i, h->num_recv_i, h->num_sent_i > 0 ?
                ( ( h->num_sent_i - h->num_recv_i ) * 100 ) / h->num_sent_i : 0 );

            if (outage_flag) {
                /* Time outage  */
                outage_ms_i = (h->num_sent_i - h->num_recv_i) * perhost_interval/100;
                fprintf( stderr, ", outage(ms) = %d", outage_ms_i );
            }
        }
        else {
            fprintf( stderr, " xmt/rcv/%%return = %d/%d/%d%%",
                h->num_sent_i, h->num_recv_i, h->num_sent_i > 0 ?
                ( ( h->num_recv_i * 100 ) / h->num_sent_i ) : 0 );
        }

        if( h->num_recv_i ) {
            avg = h->total_time_i / h->num_recv_i;
            fprintf( stderr, ", min/avg/max = %s", sprint_tm( h->min_reply_i ) );
            fprintf( stderr, "/%s", sprint_tm( avg ) );
            fprintf( stderr, "/%s", sprint_tm( h->max_reply_i ) );
        
        }
        
        fprintf( stderr, "\n" );
        h->num_sent_i = h->num_recv_i = h->max_reply_i =
            h->min_reply_i = h->total_time_i = 0;
    
    }
} /* print_per_system_splits() */


/************************************************************

  Function: print_global_stats

*************************************************************

  Inputs:  void (none)

  Description:
  

************************************************************/

void print_global_stats( void )
{
    fflush( stdout );
    fprintf( stderr, "\n" );
    fprintf( stderr, " %7d targets\n", num_hosts );
    fprintf( stderr, " %7d alive\n", num_alive );
    fprintf( stderr, " %7d unreachable\n" ,num_unreachable );
    fprintf( stderr, " %7d unknown addresses\n", num_noaddress );
    fprintf( stderr, "\n" );
    fprintf( stderr, " %7d timeouts (waiting for response)\n", num_timeout );
    fprintf( stderr, " %7d ICMP Echos sent\n", num_pingsent );
    fprintf( stderr, " %7d ICMP Echo Replies received\n", num_pingreceived );
    fprintf( stderr, " %7d other ICMP received\n", num_othericmprcvd );
    fprintf( stderr, "\n" );

    if( total_replies == 0 )
    {
        min_reply = 0;
        max_reply = 0;
        total_replies = 1;
        sum_replies = 0;
    
    }/* IF */

    fprintf( stderr, " %s ms (min round trip time)\n", sprint_tm( min_reply ) );
    fprintf( stderr, " %s ms (avg round trip time)\n",
        sprint_tm( ( int )( sum_replies / total_replies ) ) );
    fprintf( stderr, " %s ms (max round trip time)\n", sprint_tm( max_reply ) );
    fprintf( stderr, " %12.3f sec (elapsed real time)\n",
        timeval_diff( &end_time, &start_time ) / 100000.0 );
    fprintf( stderr, "\n" );

} /* print_global_stats() */


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

int send_ping( int s, HOST_ENTRY *h )
{
    int n;
    int myseq;
    int ret = 1;

    gettimeofday( &h->last_send_time, &tz );
    myseq = seqmap_add(h->i, h->num_sent, &h->last_send_time);

#if defined(DEBUG) || defined(_DEBUG)
    if( trace_flag )
        printf( "sending [%d] to %s\n", h->num_sent, h->host );
#endif /* DEBUG || _DEBUG */

    n = socket_sendto_ping(s, (struct sockaddr *) &h->saddr, h->saddr_len, myseq, ident);

    if(
        (n < 0)
#if defined( EHOSTDOWN )
        && errno != EHOSTDOWN
#endif
    ) {
        if( verbose_flag ) {
            print_warning( "%s: error while sending ping: %s\n", h->host, strerror( errno ) );
        }
        
        if( !loop_flag )
            h->resp_times[h->num_sent] = RESP_ERROR;

        ret = 0;
    }
    else {
        /* mark this trial as outstanding */
        if( !loop_flag )
            h->resp_times[h->num_sent] = RESP_WAITING;

#if defined( DEBUG ) || defined( _DEBUG )
        if( sent_times_flag )
            h->sent_times[h->num_sent] = timeval_diff( &h->last_send_time, &start_time );
#endif
    }

    h->num_sent++;
    h->num_sent_i++;
    h->waiting++;
    num_pingsent++;
    last_send_time = h->last_send_time;
    h->discard_next_recv_i=0;

    return(ret);
}

int wait_on_socket(int socket, struct timeval *timeout)
{
    int nfound;
    fd_set readset, writeset;

select_again:
    FD_ZERO( &readset );
    FD_ZERO( &writeset );
    FD_SET( s, &readset );

    nfound = select(socket + 1, &readset, &writeset, NULL, timeout);
    if(nfound < 0) {
        if(errno == EINTR) {
            /* interrupted system call: redo the select */
            goto select_again;
        }
        else {
            perror("select");
        }
    }

    if(nfound == 0)
        return 0;
    else
        return 1;
}

int receive_reply(int socket,
                  struct timeval    *timeout,
                  struct timeval    *reply_timestamp,
                  struct sockaddr   *reply_src_addr,
                  size_t            reply_src_addr_len,
                  char              *reply_buf,
                  size_t            reply_buf_len)
{
    int recv_len;

    // Wait for input on the socket
    if(timeout && !wait_on_socket(socket, timeout)) {
        return 0;   /* timeout */
    }

    // Receive data
    {
        static unsigned char msg_control[40];
        struct iovec msg_iov = {
            reply_buf,
            reply_buf_len
        };
        struct msghdr recv_msghdr = {
            reply_src_addr,
            reply_src_addr_len,
            &msg_iov,
            1,
            &msg_control,
            sizeof(msg_control),
            0
        };
        int timestamp_set = 0;

        recv_len = recvmsg(socket, &recv_msghdr, 0);
        if(recv_len <= 0) {
            return 0;
        }

#if HAVE_SO_TIMESTAMP 
	// ancilliary data
	struct cmsghdr *cmsg;
        for(cmsg = CMSG_FIRSTHDR(&recv_msghdr);
            cmsg != NULL;
            cmsg = CMSG_NXTHDR(&recv_msghdr, cmsg))
        {
            if(cmsg->cmsg_level == SOL_SOCKET &&
               cmsg->cmsg_type == SCM_TIMESTAMP)
            {
                memcpy(reply_timestamp, CMSG_DATA(cmsg), sizeof(*reply_timestamp));
                timestamp_set = 1;
            }
        }
#endif

        if(! timestamp_set) {
            gettimeofday(reply_timestamp, NULL);
        }
    }

    return recv_len;
}

int wait_for_reply(long wait_time)
{
    int result;
    static char buffer[4096];
    struct sockaddr_storage response_addr;
    int hlen = 0;
    FPING_ICMPHDR *icp;
    int n, avg;
    HOST_ENTRY *h;
    long this_reply;
    int this_count;
    struct timeval *sent_time;
    struct timeval recv_time;
    SEQMAP_VALUE *seqmap_value;
#ifndef IPV6
    struct ip *ip;
#endif

    // receive packet
    {
        struct timeval to;
        if(wait_time) {
            if(wait_time < 100000) {
                to.tv_sec = 0;
                to.tv_usec = wait_time * 10;
            }
            else {
                to.tv_sec = wait_time / 100000 ;
                to.tv_usec = (wait_time % 100000) * 10 ;
            }
        }

        result = receive_reply(s,                            // socket
                                wait_time ? &to : NULL,       // timeout
                                &recv_time,                   // reply_timestamp
                                (struct sockaddr *) &response_addr, // reply_src_addr
                                sizeof(response_addr),        // reply_src_addr_len
                                buffer,                       // reply_buf
                                sizeof(buffer)                // reply_buf_len
                            );

        if(result <= 0) {
            return 0;
        }
    }

#if defined( DEBUG ) || defined( _DEBUG )
    if( randomly_lose_flag )
    {
        if( ( random() & 0x07 ) <= lose_factor )
            return 0;

    }
#endif

#ifndef IPV6
    ip = ( struct ip* )buffer;
#if defined( __alpha__ ) && __STDC__ && !defined( __GLIBC__ )
    /* The alpha headers are decidedly broken.
     * Using an ANSI compiler, it provides ip_vhl instead of ip_hl and
     * ip_v.  So, to get ip_hl, we mask off the bottom four bits.
     */
    hlen = ( ip->ip_vhl & 0x0F ) << 2;
#else
    hlen = ip->ip_hl << 2;
#endif /* defined(__alpha__) && __STDC__ */
    if( result < hlen + ICMP_MINLEN )
#else
    if( result < sizeof(FPING_ICMPHDR) )
#endif
    {
        if( verbose_flag )
        {
            char buf[INET6_ADDRSTRLEN];
            getnameinfo((struct sockaddr *)&response_addr, sizeof(response_addr), buf, INET6_ADDRSTRLEN, NULL, 0, NI_NUMERICHOST);
            printf( "received packet too short for ICMP (%d bytes from %s)\n", result, buf);
        }
        return( 1 ); /* too short */ 
    }/* IF */

    gettimeofday( &current_time, &tz );

    icp = ( FPING_ICMPHDR* )( buffer + hlen );
#ifndef IPV6
    if( icp->icmp_type != ICMP_ECHOREPLY )
#else
    if( icp->icmp6_type != ICMP6_ECHO_REPLY )
#endif
    {
        /* handle some problem */
        if( handle_random_icmp( icp, (struct sockaddr *)&response_addr, sizeof(response_addr) ) )
            num_othericmprcvd++;
        return 1;
    }/* IF */

#ifndef IPV6
    if( ntohs(icp->icmp_id) != ident )
#else
    if( ntohs(icp->icmp6_id) != ident )
#endif
        return 1; /* packet received, but not the one we are looking for! */

#ifndef IPV6
    seqmap_value = seqmap_fetch(ntohs(icp->icmp_seq), &current_time);
#else
    seqmap_value = seqmap_fetch(ntohs(icp->icmp6_seq), &current_time);
#endif
    if(seqmap_value == NULL) {
        return 1;
    }

    num_pingreceived++;

    n = seqmap_value->host_nr;
    h = table[n];
    sent_time  = &seqmap_value->ping_ts;
    this_count = seqmap_value->ping_count;
    this_reply = timeval_diff( &recv_time, sent_time );

    if( loop_flag || h->resp_times[this_count] == RESP_WAITING )
    {
        /* only for non-duplicates: */
        h->waiting = 0;
        h->timeout = timeout;
        h->num_recv++;

        if(h->discard_next_recv_i) {
            h->discard_next_recv_i=0;
        }
        else {
            h->num_recv_i++;
            if( !h->max_reply_i || this_reply > h->max_reply_i ) h->max_reply_i = this_reply;
            if( !h->min_reply_i || this_reply < h->min_reply_i ) h->min_reply_i = this_reply;
            h->total_time_i += this_reply;
        }

        if( !max_reply      || this_reply > max_reply ) max_reply = this_reply;
        if( !min_reply      || this_reply < min_reply ) min_reply = this_reply;
        if( !h->max_reply   || this_reply > h->max_reply ) h->max_reply = this_reply;
        if( !h->min_reply   || this_reply < h->min_reply ) h->min_reply = this_reply;
        sum_replies += this_reply;
        h->total_time += this_reply;
        total_replies++;
    }

    /* received ping is cool, so process it */
    h->num_recv_total++;

#if defined( DEBUG ) || defined( _DEBUG )
    if( trace_flag ) 
        printf( "received [%d] from %s\n", this_count, h->host );
#endif /* DEBUG || _DEBUG */

    /* note reply time in array, probably */
    if( !loop_flag )
    {
        if( ( this_count >= 0 ) && ( this_count < trials ) )
        {
            if( h->resp_times[this_count] >= 0 )
            {
                if( !per_recv_flag )
                {
                    fprintf( stderr, "%s : duplicate for [%d], %d bytes, %s ms",
                        h->host, this_count, result, sprint_tm( this_reply ) );

                    if(addr_cmp((struct sockaddr *)&response_addr, (struct sockaddr *)&h->saddr)) {
                        char buf[INET6_ADDRSTRLEN];
                        getnameinfo((struct sockaddr *)&response_addr, sizeof(response_addr), buf, INET6_ADDRSTRLEN, NULL, 0, NI_NUMERICHOST);
                        fprintf( stderr, " [<- %s]", buf);
                    }
                    fprintf( stderr, "\n" );
    
                }/* IF */
            }/* IF */
            else
                h->resp_times[this_count] = this_reply;
        
        }/* IF */
        else
        {
            /* count is out of bounds?? */
            fprintf( stderr, "%s : duplicate for [%d], %d bytes, %s ms\n",
                h->host, this_count, result, sprint_tm( this_reply ) );
        
        }/* ELSE */
    }/* IF */

    if( h->num_recv == 1 )
    {
        num_alive++;
        if( verbose_flag || alive_flag )
        {
            printf( "%s", h->host );

            if( verbose_flag )
                printf( " is alive" );

            if( elapsed_flag )
                printf( " (%s ms)", sprint_tm( this_reply ) );

            if(addr_cmp((struct sockaddr *)&response_addr, (struct sockaddr *)&h->saddr)) {
                char buf[INET6_ADDRSTRLEN];
                getnameinfo((struct sockaddr *)&response_addr, sizeof(response_addr), buf, INET6_ADDRSTRLEN, NULL, 0, NI_NUMERICHOST);
                fprintf( stderr, " [<- %s]", buf);
            }

            printf( "\n" );
        
        }/* IF */
    }/* IF */

    if( per_recv_flag )
    {
        if(timestamp_flag) {
            printf("[%lu.%06lu] ",
                 (unsigned long)recv_time.tv_sec,
                 (unsigned long)recv_time.tv_usec);
        }
        avg = h->total_time / h->num_recv;
        printf( "%s%s : [%d], %d bytes, %s ms",
            h->host, h->pad, this_count, result, sprint_tm( this_reply ) );
        printf( " (%s avg, ", sprint_tm( avg ) );
    
        if( h->num_recv <= h->num_sent ) {
            printf( "%d%% loss)",
                ( ( h->num_sent - h->num_recv ) * 100 ) / h->num_sent );

        }
        else {
            printf( "%d%% return)",
                ( h->num_recv_total * 100 ) / h->num_sent );
        
        }

        if(addr_cmp((struct sockaddr *)&response_addr, (struct sockaddr *)&h->saddr)) {
            char buf[INET6_ADDRSTRLEN];
            getnameinfo((struct sockaddr *)&response_addr, sizeof(response_addr), buf, INET6_ADDRSTRLEN, NULL, 0, NI_NUMERICHOST);
            fprintf( stderr, " [<- %s]", buf);
        }
        
        printf( "\n" );
    
    }/* IF */

    /* remove this job, if we are done */
    if((count_flag && h->num_recv >= count) ||
       (!loop_flag && !count_flag && h->num_recv))
    {
        remove_job(h);
    }
    
    fflush( stdout );
    return num_jobs;

} /* wait_for_reply() */

/************************************************************

  Function: handle_random_icmp

************************************************************/

int handle_random_icmp(FPING_ICMPHDR *p, struct sockaddr *addr, socklen_t addr_len)
{
    FPING_ICMPHDR *sent_icmp;
    unsigned char *c;
    HOST_ENTRY *h;
    SEQMAP_VALUE *seqmap_value;
    char addr_ascii[INET6_ADDRSTRLEN];
    unsigned short icmp_type;
    unsigned short icmp_code;
    unsigned short sent_icmp_type;
    unsigned short sent_icmp_seq;
    unsigned short sent_icmp_id;

    getnameinfo((struct sockaddr *) addr, addr_len, addr_ascii, INET6_ADDRSTRLEN, NULL, 0, NI_NUMERICHOST);

    c = ( unsigned char* )p;

    sent_icmp = ( FPING_ICMPHDR* )( c + 28 );
#ifndef IPV6
    icmp_type = p->icmp_type;
    icmp_code = p->icmp_code;
    sent_icmp_type = sent_icmp->icmp_type;
    sent_icmp_seq  = sent_icmp->icmp_seq;
    sent_icmp_id   = sent_icmp->icmp_id;
#else
    icmp_type = p->icmp6_type;
    icmp_code = p->icmp6_code;
    sent_icmp_type = sent_icmp->icmp6_type;
    sent_icmp_seq  = sent_icmp->icmp6_seq;
    sent_icmp_id   = sent_icmp->icmp6_id;
#endif

    switch(icmp_type)
    {
    case ICMP_UNREACH:
        seqmap_value = seqmap_fetch(ntohs(sent_icmp_seq), &current_time);

        if( ( sent_icmp_type == ICMP_ECHO ) &&
            ( ntohs(sent_icmp_id) == ident ) &&
            ( seqmap_value != NULL ) )
        {
            /* this is a response to a ping we sent */
            h = table[ntohs(sent_icmp_seq) % num_hosts];
            
            if( icmp_code > ICMP_UNREACH_MAXTYPE ) {
                print_warning("ICMP Unreachable (Invalid Code) from %s for ICMP Echo sent to %s",
                    addr_ascii, h->host );
            }
            else {
                print_warning("%s from %s for ICMP Echo sent to %s",
                        icmp_unreach_str[icmp_code], addr_ascii, h->host);
            }

            if( inet_addr( h->host ) == INADDR_NONE )
                print_warning(" (%s)", addr_ascii);
            
            print_warning("\n" );
        }
        return 1;

    case ICMP_SOURCEQUENCH:
    case ICMP_REDIRECT:
    case ICMP_TIMXCEED:
    case ICMP_PARAMPROB:
        seqmap_value = seqmap_fetch(ntohs(sent_icmp_seq), &current_time);

        if( ( sent_icmp_type == ICMP_ECHO ) &&
            ( ntohs(sent_icmp_id) == ident ) &&
            ( seqmap_value != NULL ) )
        {
            /* this is a response to a ping we sent */
            h = table[ntohs(sent_icmp_seq) % num_hosts];
            if(icmp_type <= ICMP_TYPE_STR_MAX) {
                print_warning("%s from %s for ICMP Echo sent to %s",
                    icmp_type_str[icmp_type], addr_ascii, h->host );
            }
            else {
                print_warning("ICMP %d from %s for ICMP Echo sent to %s",
                    icmp_type, addr_ascii, h->host );
            }
      
            if( inet_addr( h->host ) == INADDR_NONE )
                print_warning(" (%s)", addr_ascii );

            print_warning( "\n" );
        
        }/* IF */

        return 2;

    /* no way to tell whether any of these are sent due to our ping */
    /* or not (shouldn't be, of course), so just discard            */
    case ICMP_TSTAMP:
    case ICMP_TSTAMPREPLY:
    case ICMP_IREQ:
    case ICMP_IREQREPLY:
    case ICMP_MASKREQ:
    case ICMP_MASKREPLY:
    default:
        return 0;
    
    }/* SWITCH */

} /* handle_random_icmp() */


/************************************************************

  Function: add_name

*************************************************************

  Inputs:  char* name

  Description:

  process input name for addition to target list
  name can turn into multiple targets via multiple interfaces (-m)
  or via NIS groups

************************************************************/

void add_name( char *name )
{
    struct addrinfo   *res0, *res, hints;
    int               ret_ga;
    char              *printname;
    char              namebuf[256];
    char              addrbuf[256];

    /* getaddrinfo */
    bzero(&hints, sizeof(struct addrinfo));
    hints.ai_flags = 0;
    hints.ai_socktype = SOCK_RAW;
#ifndef IPV6
    hints.ai_family = AF_INET;
    hints.ai_protocol = IPPROTO_ICMP;
#else
    hints.ai_family = AF_INET6;
    hints.ai_protocol = IPPROTO_ICMPV6;
#endif
    ret_ga = getaddrinfo(name, NULL, &hints, &res0);
    if (ret_ga) {
        if(!quiet_flag)
            print_warning("%s: %s\n", name, gai_strerror(ret_ga));
        num_noaddress++;
        return; 
    }

    // NOTE: we could/should loop with res on all addresses like this:
    // for (res = res0; res; res = res->ai_next) {
    // We don't do it yet, however, because is is an incompatible change
    // (need to implement a separate option for this)

    for (res = res0; res; res = 0) {
        /* name_flag: addr -> name lookup requested) */
        if(!name_flag) {
            printname = name;
        }
        else {
            int ret;
            ret = getnameinfo(res->ai_addr, res->ai_addrlen, namebuf,
                              sizeof(namebuf)/sizeof(char), NULL, 0, 0);
            if (ret) {
                if(!quiet_flag) {
                    print_warning("%s: %s\n", name, gai_strerror(ret_ga));
                }
                num_noaddress++;
                return; 
            }
            printname = namebuf;
        }

        /* addr_flag: name -> addr lookup requested */
        if(addr_flag) {
            int ret;
            ret = getnameinfo(res->ai_addr, res->ai_addrlen, addrbuf,
                              sizeof(addrbuf)/sizeof(char), NULL, 0, NI_NUMERICHOST);
            if (ret) {
                if(!quiet_flag) {
                    print_warning("%s: %s\n", name, gai_strerror(ret_ga));
                }
                num_noaddress++;
                return; 
            }

            if(name_flag) {
                char nameaddrbuf[512];
                snprintf(nameaddrbuf, sizeof(nameaddrbuf)/sizeof(char), "%s (%s)", printname, addrbuf);
                add_addr(name, nameaddrbuf, res->ai_addr, res->ai_addrlen);
            }
            else {
                add_addr(name, addrbuf, res->ai_addr, res->ai_addrlen);
            }
        }
        else {
            add_addr(name, printname, res->ai_addr, res->ai_addrlen);
        }

        return;
    }
} /* add_name() */


/************************************************************

  Function: add_addr

*************************************************************

  Description:

  add address to linked list of targets to be pinged
  assume memory for *name and *host is ours!!!

************************************************************/

void add_addr( char *name, char *host, struct sockaddr *ipaddr, socklen_t ipaddr_len )
{
    HOST_ENTRY *p;
    int n, *i;

    p = ( HOST_ENTRY* )malloc( sizeof( HOST_ENTRY ) );
    if( !p )
        crash_and_burn( "can't allocate HOST_ENTRY" );

    memset( ( char* ) p, 0, sizeof( HOST_ENTRY ) );

    p->name = strdup(name);
    p->host = strdup(host);
    memcpy(&p->saddr, ipaddr, ipaddr_len);
    p->saddr_len = ipaddr_len;
    p->timeout = timeout;
    p->running = 1;
    p->min_reply = 0;

    if(netdata_flag) {
        char *s = p->name;
        while(*s) {
            if(!isalnum(*s)) *s = '_';
            s++;
        }
    }

    if( strlen( p->host ) > max_hostname_len )
        max_hostname_len = strlen( p->host );

    /* array for response time results */
    if( !loop_flag )
    {
        i = ( int* )malloc( trials * sizeof( int ) );
        if( !i )
            crash_and_burn( "can't allocate resp_times array" );
        
        for( n = 1; n < trials; n++ )
            i[n] = RESP_UNUSED;
        
        p->resp_times = i;

    }/* IF */

#if defined( DEBUG ) || defined( _DEBUG )
    /* likewise for sent times */
    if( sent_times_flag )
    {
        i = ( int* )malloc( trials * sizeof( int ) );
        if( !i )
            crash_and_burn( "can't allocate sent_times array" );
    
        for( n = 1; n < trials; n++ )
            i[n] = RESP_UNUSED;
        
        p->sent_times = i;
    
    }/* IF */
#endif /* DEBUG || _DEBUG */

    /* schedule first ping */
    p->ev_type = EV_TYPE_PING;
    p->ev_time.tv_sec  = 0;
    p->ev_time.tv_usec = 0;
    ev_enqueue(p);

    num_hosts++;

} /* add_addr() */


/************************************************************

  Function: remove_job

*************************************************************

  Inputs:  HOST_ENTRY *h

  Description:

************************************************************/

void remove_job( HOST_ENTRY *h )
{
#if defined( DEBUG ) || defined( _DEBUG )
    if( trace_flag )
        printf( "removing job for %s\n", h->host );
#endif /* DEBUG || _DEBUG */

    h->running = 0;
    h->waiting = 0;
    --num_jobs;

    ev_remove(h);

} /* remove_job() */

/************************************************************

  Function: crash_and_burn

*************************************************************

  Inputs:  char* message

  Description:

************************************************************/
  
void crash_and_burn( char *message )
{
    if( verbose_flag )
        fprintf( stderr, "%s: %s\n", prog, message );
    
    exit( 4 );

} /* crash_and_burn() */


/************************************************************

  Function: errno_crash_and_burn

*************************************************************

  Inputs:  char* message

  Description:

************************************************************/

void errno_crash_and_burn( char *message )
{
    fprintf( stderr, "%s: %s : %s\n", prog, message, strerror( errno ) );
    exit( 4 );
} /* errno_crash_and_burn() */


/************************************************************

  Function: print_warning

  Description: fprintf(stderr, ...), unless running with -q

*************************************************************/

void print_warning(char *format, ...) {
    va_list args;
    if(!quiet_flag) {
        va_start(args, format );
        vfprintf(stderr, format, args);
        va_end(args);
    }
}



/************************************************************

  Function: timeval_diff

*************************************************************

  Inputs:  struct timeval *a, struct timeval *b

  Returns:  long

  Description:

  timeval_diff now returns result in hundredths of milliseconds
  ie, tens of microseconds                                    

************************************************************/

long timeval_diff( struct timeval *a, struct timeval *b )
{
    long sec_diff = a->tv_sec - b->tv_sec;
    if(sec_diff == 0) {
        return (a->tv_usec - b->tv_usec) / 10;
    }
    else if(sec_diff < 100) {
        return (sec_diff * 1000000 + a->tv_usec - b->tv_usec) / 10;
    }
    else {
        /* For such large differences, we don't really care about the microseconds... */
        return sec_diff * 100000;
    }
} /* timeval_diff() */

/************************************************************

  Function: timeval_add

*************************************************************/
void timeval_add(struct timeval *a, long t_10u)
{
    t_10u *= 10;
    a->tv_sec += (t_10u + a->tv_usec) / 1000000;
    a->tv_usec = (t_10u + a->tv_usec) % 1000000;
}

/************************************************************

  Function: sprint_tm

*************************************************************

  Inputs:  int t

  Returns:  char*

  Description:

  render time into a string with three digits of precision
  input is in tens of microseconds

************************************************************/

char * sprint_tm( int t )
{
    static char buf[10];

    if( t < 0 ) {
        /* negative (unexpected) */
        sprintf( buf, "%.2g", (double) t / 100 );
    }
    else if( t < 100 ) {
        /* <= 0.99 ms */
        sprintf( buf, "0.%02d", t );
    }
    else if( t < 1000 ) {
        /* 1.00 - 9.99 ms */
        sprintf( buf, "%d.%02d", t / 100, t % 100 );
    }
    else if( t < 10000 ) {
        /* 10.0 - 99.9 ms */
        sprintf( buf, "%d.%d", t / 100, ( t % 100 ) / 10 );
    }
    else if( t < 100000000 ) {
        /* 100 - 1'000'000 ms */
        sprintf( buf, "%d", t / 100 );
    }
    else {
        sprintf( buf, "%.2e", (double) (t / 100) );
    }

    return( buf );
}

/************************************************************
  Function: recvfrom_wto
*************************************************************
  Description:

  receive with timeout
  returns length of data read or -1 if timeout
  crash_and_burn on any other errrors
************************************************************/

int recvfrom_wto( int s, char *buf, int len, struct sockaddr *saddr, socklen_t *saddr_len, long timo )
{
    int nfound, n;
    struct timeval to;
    fd_set readset, writeset;

select_again:
    if(timo < 100000) {
        to.tv_sec = 0;
        to.tv_usec = timo * 10;
    }
    else {
        to.tv_sec = timo / 100000 ;
        to.tv_usec = (timo % 100000) * 10 ;
    }

    FD_ZERO( &readset );
    FD_ZERO( &writeset );
    FD_SET( s, &readset );

    nfound = select( s + 1, &readset, &writeset, NULL, &to );
    if(nfound < 0) {
        if(errno == EINTR) {
            /* interrupted system call: redo the select */
            goto select_again;
        }
        else {
            errno_crash_and_burn( "select" );
        }
    }

    if( nfound == 0 )
        return -1;      /* timeout */

    // recvfrom(int socket, void *restrict buffer, size_t length, int flags, struct sockaddr *restrict address, socklen_t *restrict address_len);
    n = recvfrom( s, buf, len, 0, saddr, saddr_len );
    if( n < 0 )
        errno_crash_and_burn( "recvfrom" );

    return n;

} /* recvfrom_wto() */

/************************************************************

  Function: addr_cmp

*************************************************************/
int addr_cmp(struct sockaddr *a, struct sockaddr *b)
{
    if(a->sa_family != b->sa_family) {
        return a->sa_family - b->sa_family;
    }
    else {
        if(a->sa_family == AF_INET) {
            return ((struct sockaddr_in *) a)->sin_addr.s_addr - ((struct sockaddr_in *) b)->sin_addr.s_addr;
        }
        else if(a->sa_family == AF_INET6) {
            return memcmp(&((struct sockaddr_in6 *) a)->sin6_addr,
                          &((struct sockaddr_in6 *) b)->sin6_addr,
                          sizeof(((struct sockaddr_in6 *) a)->sin6_addr));
        }
    }

    return 0;
}

/************************************************************

  Function: ev_enqueue

  Enqueue a host that needs to be pinged, but not before the time
  written in h->ev_time.

  The queue is sorted, so that ev_first always points to the host
  that should be pinged first.

  We start scanning the queue from the tail, because we assume
  that new events mostly get inserted with a event time higher
  than the others.

*************************************************************/
void ev_enqueue(HOST_ENTRY  *h)
{
    HOST_ENTRY *i;
    HOST_ENTRY *i_prev;

#if defined( DEBUG ) || defined( _DEBUG )
    if( trace_flag ) {
        long st = timeval_diff(&h->ev_time, &current_time);
        fprintf(stderr, "Enqueue: host=%s, when=%d ms (%d, %d)\n", h->host, st/100, h->ev_time.tv_sec, h->ev_time.tv_usec);
    }
#endif

    /* Empty list */
    if(ev_last == NULL) {
        h->ev_next = NULL;
        h->ev_prev = NULL;
        ev_first = h;
        ev_last = h;
        return;
    }

    /* Insert on tail? */
    if(h->ev_time.tv_sec > ev_last->ev_time.tv_sec ||
       (h->ev_time.tv_sec == ev_last->ev_time.tv_sec &&
        h->ev_time.tv_usec >= ev_last->ev_time.tv_usec))
    {
        h->ev_next = NULL;
        h->ev_prev = ev_last;
        ev_last->ev_next = h;
        ev_last = h;
        return;
    }

    /* Find insertion point */
    i = ev_last;
    while(1) {
        i_prev = i->ev_prev;
        if(i_prev == NULL || 
           h->ev_time.tv_sec > i_prev->ev_time.tv_sec ||
           (h->ev_time.tv_sec == i_prev->ev_time.tv_sec &&
            h->ev_time.tv_usec >= i_prev->ev_time.tv_usec))
        {
            h->ev_prev = i_prev;
            h->ev_next = i;
            i->ev_prev = h;
            if(i_prev != NULL) {
                i_prev->ev_next = h;
            }
            else {
                ev_first = h;
            }
            return;
        }
        i = i_prev;
    }
}

/************************************************************

  Function: ev_dequeue

*************************************************************/
HOST_ENTRY *ev_dequeue()
{
    HOST_ENTRY *dequeued;

    if(ev_first == NULL) {
        return NULL;
    }
    dequeued = ev_first;
    ev_remove(dequeued);

    return dequeued;
}

/************************************************************

  Function: ev_remove

*************************************************************/
void ev_remove(HOST_ENTRY *h)
{
    if(ev_first == h) {
        ev_first = h->ev_next;
    }
    if(ev_last == h) {
        ev_last = h->ev_prev;
    }
    if(h->ev_prev) {
        h->ev_prev->ev_next = h->ev_next;
    }
    if(h->ev_next) {
        h->ev_next->ev_prev = h->ev_prev;
    }
    h->ev_prev = NULL;
    h->ev_next = NULL;
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
    fprintf(out, "\n" );
    fprintf(out, "Usage: %s [options] [targets...]\n", prog );
    fprintf(out, "   -a         show targets that are alive\n" );
    fprintf(out, "   -A         show targets by address\n" );
    fprintf(out, "   -b n       amount of ping data to send, in bytes (default %d)\n", DEFAULT_PING_DATA_SIZE);
    fprintf(out, "   -B f       set exponential backoff factor to f\n" );
    fprintf(out, "   -c n       count of pings to send to each target (default %d)\n", count );  
    fprintf(out, "   -C n       same as -c, report results in verbose format\n" );
    fprintf(out, "   -D         print timestamp before each output line\n" );
    fprintf(out, "   -e         show elapsed time on return packets\n" );
    fprintf(out, "   -f file    read list of targets from a file ( - means stdin) (only if no -g specified)\n" );
    fprintf(out, "   -g         generate target list (only if no -f specified)\n" );
    fprintf(out, "                (specify the start and end IP in the target list, or supply a IP netmask)\n" );
    fprintf(out, "                (ex. %s -g 192.168.1.0 192.168.1.255 or %s -g 192.168.1.0/24)\n", prog, prog );
    fprintf(out, "   -H n       Set the IP TTL value (Time To Live hops)\n");
    fprintf(out, "   -i n       interval between sending ping packets (in millisec) (default %d)\n", interval / 100 );
#ifdef SO_BINDTODEVICE
        fprintf(out, "   -I if      bind to a particular interface\n");
#endif
    fprintf(out, "   -l         loop sending pings forever\n" );
    fprintf(out, "   -m         ping multiple interfaces on target host\n" );
    fprintf(out, "   -M         set the Don't Fragment flag\n" );
    fprintf(out, "   -n         show targets by name (-d is equivalent)\n" );
    fprintf(out, "   -N         output compatible for netdata (-l -Q are required)\n" );
    fprintf(out, "   -o         show the accumulated outage time (lost packets * packet interval)\n" );
    fprintf(out, "   -O n       set the type of service (tos) flag on the ICMP packets\n" );
    fprintf(out, "   -p n       interval between ping packets to one target (in millisec)\n" );
    fprintf(out, "                (in looping and counting modes, default %d)\n", perhost_interval / 100 );
    fprintf(out, "   -q         quiet (don't show per-target/per-ping results)\n" );
    fprintf(out, "   -Q n       same as -q, but show summary every n seconds\n" );
    fprintf(out, "   -r n       number of retries (default %d)\n", DEFAULT_RETRY );
    fprintf(out, "   -R         random packet data (to foil link data compression)\n" );
    fprintf(out, "   -s         print final stats\n" );
    fprintf(out, "   -S addr    set source address\n" );
    fprintf(out, "   -t n       individual target initial timeout (in millisec) (default %d)\n", timeout / 100 );
    fprintf(out, "   -T n       ignored (for compatibility with fping 2.4)\n");
    fprintf(out, "   -u         show targets that are unreachable\n" );
    fprintf(out, "   -v         show version\n" );
    fprintf(out, "   targets    list of targets to check (if no -f specified)\n" );
    fprintf(out, "\n");
    exit(is_error);
} /* usage() */
