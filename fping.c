/* 
 * fping: fast-ping, file-ping, favorite-ping, funky-ping
 *
 *   Ping a list of target hosts in a round robin fashion.
 *   A better ping overall.
 *
 * VIEWING NOTES:
 * 
 * This file was formatted with tab indents at a tab stop of 4.
 *
 * It is highly recommended that your editor is set to this
 * tab stop setting for viewing and editing.
 *
 * fping website:  http://www.fping.com
 *
 *
 *
 * Current maintainers of fping:
 *
 * David Papp
 * Suggestions and patches, please email david@remote.net
 *
 *
 *
 * Original author:  Roland Schemers  <schemers@stanford.edu>
 * IPv6 Support:     Jeroen Massar    <jeroen@unfix.org / jeroen@ipng.nl>
 * Bugfixes, byte order & senseful seq.-numbers: Stephan Fuhrmann (stephan.fuhrmann AT 1und1.de)
 *
 *
 * RCS header information no longer used.  It has been moved to the
 * ChangeLog file.
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

#ifndef _NO_PROTO
#if !__STDC__ && !defined( __cplusplus ) && !defined( FUNCPROTO ) \
                                                 && !defined( _POSIX_SOURCE )
#define _NO_PROTO
#endif /* __STDC__ */
#endif /* _NO_PROTO */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

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

#include <netinet/in.h>

#include "config.h"

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

/* RS6000 has sys/select.h */
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif /* HAVE_SYS_SELECT_H */

#endif /* WIN32 */

#include "options.h"

/*** externals ***/

extern char *optarg;
extern int optind,opterr;
extern int h_errno;

#ifdef __cplusplus
}
#endif /* __cplusplus */

/*** Constants ***/

#define REV_DATE	"2002/01/16 00:33:42"
#define EMAIL		"david@remote.net"

/*** Ping packet defines ***/

/* data added after ICMP header for our nefarious purposes */

typedef struct ping_data
{
     int                  ping_count;         /* counts up to -c count or 1 */
     struct timeval       ping_ts;            /* time sent */

} PING_DATA;

#define MIN_PING_DATA	sizeof( PING_DATA )
#define	MAX_IP_PACKET	65536	/* (theoretical) max IP packet size */
#define SIZE_IP_HDR		20
#ifndef IPV6
#define SIZE_ICMP_HDR	ICMP_MINLEN		/* from ip_icmp.h */
#else
#define SIZE_ICMP_HDR	sizeof(FPING_ICMPHDR)
#endif
#define MAX_PING_DATA	( MAX_IP_PACKET - SIZE_IP_HDR - SIZE_ICMP_HDR )

/* sized so as to be like traditional ping */
#define DEFAULT_PING_DATA_SIZE	( MIN_PING_DATA + 44 ) 

/* maxima and minima */
#define MAX_COUNT				10000
#define MIN_INTERVAL			10		/* in millisec */
#define MIN_PERHOST_INTERVAL	20		/* in millisec */
#define MIN_TIMEOUT				50		/* in millisec */
#define MAX_RETRY				20

/* response time array flags */
#define RESP_WAITING	-1
#define RESP_UNUSED		-2

/* debugging flags */
#if defined( DEBUG ) || defined( _DEBUG )
#define DBG_TRACE				1
#define DBG_SENT_TIMES			2
#define DBG_RANDOM_LOSE_FEW		4
#define DBG_RANDOM_LOSE_MANY	8
#define DBG_PRINT_PER_SYSTEM	16
#define DBG_REPORT_ALL_RTTS		32
#endif /* DEBUG || _DEBUG */

/* Long names for ICMP packet types */
char *icmp_type_str[19] =
{
	"ICMP Echo Reply",			/* 0 */
	"",
	"",
	"ICMP Unreachable",			/* 3 */
	"ICMP Source Quench",		/* 4 */
	"ICMP Redirect",			/* 5 */
	"",
	"",
	"ICMP Echo",				/* 8 */
	"",
	"",
	"ICMP Time Exceeded",		/* 11 */
	"ICMP Parameter Problem",	/* 12 */
	"ICMP Timestamp Request",	/* 13 */
	"ICMP Timestamp Reply",		/* 14 */
	"ICMP Information Request",	/* 15 */
	"ICMP Information Reply",	/* 16 */
	"ICMP Mask Request",		/* 17 */
	"ICMP Mask Reply"			/* 18 */
};

char *icmp_unreach_str[16] =
{
	"ICMP Network Unreachable",										/* 0 */
	"ICMP Host Unreachable",										/* 1 */
	"ICMP Protocol Unreachable",									/* 2 */
	"ICMP Port Unreachable",										/* 3 */
	"ICMP Unreachable (Fragmentation Needed)",						/* 4 */
	"ICMP Unreachable (Source Route Failed)",						/* 5 */
	"ICMP Unreachable (Destination Network Unknown)",				/* 6 */
	"ICMP Unreachable (Destination Host Unknown)",					/* 7 */
	"ICMP Unreachable (Source Host Isolated)",						/* 8 */
	"ICMP Unreachable (Communication with Network Prohibited)",		/* 9 */
	"ICMP Unreachable (Communication with Host Prohibited)",		/* 10 */
	"ICMP Unreachable (Network Unreachable For Type Of Service)",	/* 11 */
	"ICMP Unreachable (Host Unreachable For Type Of Service)",		/* 12 */
	"ICMP Unreachable (Communication Administratively Prohibited)",	/* 13 */
	"ICMP Unreachable (Host Precedence Violation)",					/* 14 */
	"ICMP Unreachable (Precedence cutoff in effect)"				/* 15 */
};

#define	ICMP_UNREACH_MAXTYPE	15
#ifndef IPV6
#define	FPING_SOCKADDR struct sockaddr_in
#define	FPING_ICMPHDR	struct icmp
#else
#define	FPING_SOCKADDR struct sockaddr_in6
#define	FPING_ICMPHDR	struct icmp6_hdr
#endif

/* entry used to keep track of each host we are pinging */

typedef struct host_entry
{
     struct host_entry    *prev,*next;        /* doubly linked list */
     int                  i;                  /* index into array */
     char                 *name;              /* name as given by user */
     char                 *host;              /* text description of host */
     char                 *pad;               /* pad to align print names */
     FPING_SOCKADDR       saddr;              /* internet address */
     int                  timeout;            /* time to wait for response */
     u_char               running;            /* unset when through sending */
     u_char               waiting;            /* waiting for response */
     struct timeval       last_send_time;     /* time of last packet sent */
     int                  num_sent;           /* number of ping packets sent */
     int                  num_recv;           /* number of pings received */
     int                  max_reply;          /* longest response time */
     int                  min_reply;          /* shortest response time */
     int                  total_time;         /* sum of response times */
     int                  num_sent_i;         /* number of ping packets sent */
     int                  num_recv_i;         /* number of pings received */
     int                  max_reply_i;        /* longest response time */
     int                  min_reply_i;        /* shortest response time */
     int                  total_time_i;       /* sum of response times */
     int                  *resp_times;        /* individual response times */
#if defined( DEBUG ) || defined( _DEBUG )
     int                  *sent_times;        /* per-sent-ping timestamp */
#endif /* DEBUG || _DEBUG */
} HOST_ENTRY;

/*** globals ***/

HOST_ENTRY *rrlist = NULL;	/* linked list of hosts be pinged */
HOST_ENTRY **table = NULL;	/* array of pointers to items in the list */
HOST_ENTRY *cursor;

char *prog;
int ident;					/* our pid */
int s;						/* socket */
u_int debugging = 0;

/* times get *100 because all times are calculated in 10 usec units, not ms */
u_int retry = DEFAULT_RETRY;
u_int timeout = DEFAULT_TIMEOUT * 100; 
u_int interval = DEFAULT_INTERVAL * 100;
u_int perhost_interval = DEFAULT_PERHOST_INTERVAL * 100;
float backoff = DEFAULT_BACKOFF_FACTOR;
u_int select_time = DEFAULT_SELECT_TIME * 100;
u_int ping_data_size = DEFAULT_PING_DATA_SIZE;
u_int ping_pkt_size;
u_int count = 1;
u_int trials;
u_int report_interval = 0;
int src_addr_present = 0;
#ifndef IPV6
struct in_addr src_addr;
#else
struct in6_addr src_addr;
#endif

/* global stats */
long max_reply = 0;
long min_reply = 1000000;
int total_replies = 0;
double sum_replies = 0;
int max_hostname_len = 0;
int num_jobs = 0;					/* number of hosts still to do */
int num_hosts;						/* total number of hosts */
int max_seq_sent = 0;				/* maximum sequence number sent so far */
int num_alive = 0,					/* total number alive */
    num_unreachable = 0,			/* total number unreachable */
    num_noaddress = 0;				/* total number of addresses not found */
int num_timeout = 0,				/* number of times select timed out */
    num_pingsent = 0,				/* total pings sent */
    num_pingreceived = 0,			/* total pings received */
    num_othericmprcvd = 0;			/* total non-echo-reply ICMP received */

struct timeval current_time;		/* current time (pseudo) */
struct timeval start_time; 
struct timeval end_time;
struct timeval last_send_time;		/* time last ping was sent */
struct timeval last_report_time;	/* time last report was printed */
struct timezone tz;

/* switches */
int generate_flag = 0;				/* flag for IP list generation */
int verbose_flag, quiet_flag, stats_flag, unreachable_flag, alive_flag;
int elapsed_flag, version_flag, count_flag, loop_flag;
int per_recv_flag, report_all_rtts_flag, name_flag, addr_flag, backoff_flag;
int multif_flag;
#if defined( DEBUG ) || defined( _DEBUG )
int randomly_lose_flag, sent_times_flag, trace_flag, print_per_system_flag;
int lose_factor;
#endif /* DEBUG || _DEBUG */

char *filename = NULL;				/* file containing hosts to ping */

/*** forward declarations ***/

#ifdef _NO_PROTO

void add_name();
void add_addr();
char *na_cat();
char *cpystr();
void crash_and_burn();
void errno_crash_and_burn();
char *get_host_by_address();
int in_cksum();
void u_sleep();
int recvfrom_wto ();
void remove_job();
void send_ping();
void usage();
int wait_for_reply();
long timeval_diff();
void print_per_system_stats();
void print_per_system_splits();
void print_global_stats();
void finish();
int handle_random_icmp();
char *sprint_tm();

#else

void add_name( char *name );
#ifndef IPV6
void add_addr( char *name, char *host, struct in_addr ipaddr );
#else
void add_addr( char *name, char *host, FPING_SOCKADDR *ipaddr );
#endif
char *na_cat( char *name, struct in_addr ipaddr );
char *cpystr( char *string );
void crash_and_burn( char *message );
void errno_crash_and_burn( char *message );
char *get_host_by_address( struct in_addr in );
int in_cksum( u_short *p, int n );
void u_sleep( int u_sec );
int recvfrom_wto ( int s, char *buf, int len, FPING_SOCKADDR *saddr, int timo );
void remove_job( HOST_ENTRY *h );
void send_ping( int s, HOST_ENTRY *h );
long timeval_diff( struct timeval *a, struct timeval *b );
void usage( void );
int wait_for_reply( void );
void print_per_system_stats( void );
void print_per_system_splits( void );
void print_global_stats( void );
void finish();
int handle_random_icmp( FPING_ICMPHDR *p, int psize, FPING_SOCKADDR *addr );
char *sprint_tm( int t );

#endif /* _NO_PROTO */

/*** function definitions ***/

/************************************************************

  Function: main

*************************************************************

  Inputs:  int argc, char** argv

  Description:
  
  Main program entry point

************************************************************/

#ifdef _NO_PROTO
int main( argc, argv )
int argc; char **argv;
#else
int main( int argc, char **argv )
#endif /* _NO_PROTO */
{
	int c, i, n;
#ifdef IPV6
	int opton = 1;
#endif
	u_int lt, ht;
	int advance;
	struct protoent *proto;
	char *buf;
	uid_t uid;
#ifndef IPV6
	struct sockaddr_in sa;
#else
	struct sockaddr_in6 sa;
#endif
	/* check if we are root */

	if( geteuid() )
	{
		fprintf( stderr,
			"This program can only be run by root, or it must be setuid root.\n" );

		exit( 3 );

	}/* IF */

	/* confirm that ICMP is available on this machine */
#ifndef IPV6
	if( ( proto = getprotobyname( "icmp" ) ) == NULL ) 
#else
	if( ( proto = getprotobyname( "ipv6-icmp" ) ) == NULL ) 
#endif
		crash_and_burn( "icmp: unknown protocol" );

	/* create raw socket for ICMP calls (ping) */
#ifndef IPV6
	s = socket( AF_INET, SOCK_RAW, proto->p_proto );
#else
	s = socket( AF_INET6, SOCK_RAW, proto->p_proto );
#endif

	if( s < 0 )
		errno_crash_and_burn( "can't create raw socket" );

#ifdef IPV6
	/*
	 * let the kernel pass extension headers of incoming packets,
	 * for privileged socket options
	 */
#ifdef IPV6_RECVHOPOPTS
		if (setsockopt(s, IPPROTO_IPV6, IPV6_RECVHOPOPTS, &opton,
		    sizeof(opton)))
			err(1, "setsockopt(IPV6_RECVHOPOPTS)");
#else  /* old adv. API */
		if (setsockopt(s, IPPROTO_IPV6, IPV6_HOPOPTS, &opton,
		    sizeof(opton)))
			err(1, "setsockopt(IPV6_HOPOPTS)");
#endif
#ifdef IPV6_RECVDSTOPTS
		if (setsockopt(s, IPPROTO_IPV6, IPV6_RECVDSTOPTS, &opton,
		    sizeof(opton)))
			err(1, "setsockopt(IPV6_RECVDSTOPTS)");
#else  /* old adv. API */
		if (setsockopt(s, IPPROTO_IPV6, IPV6_DSTOPTS, &opton,
		    sizeof(opton)))
			err(1, "setsockopt(IPV6_DSTOPTS)");
#endif
#ifdef IPV6_RECVRTHDRDSTOPTS
		if (setsockopt(s, IPPROTO_IPV6, IPV6_RECVRTHDRDSTOPTS, &opton,
		    sizeof(opton)))
			err(1, "setsockopt(IPV6_RECVRTHDRDSTOPTS)");
#endif
#ifdef IPV6_RECVRTHDR
		if (setsockopt(s, IPPROTO_IPV6, IPV6_RECVRTHDR, &opton,
		    sizeof(opton)))
			err(1, "setsockopt(IPV6_RECVRTHDR)");
#else  /* old adv. API */
		if (setsockopt(s, IPPROTO_IPV6, IPV6_RTHDR, &opton,
		    sizeof(opton)))
			err(1, "setsockopt(IPV6_RTHDR)");
#endif
#ifndef USE_SIN6_SCOPE_ID
#ifdef IPV6_RECVPKTINFO
		if (setsockopt(s, IPPROTO_IPV6, IPV6_RECVPKTINFO, &opton,
		    sizeof(opton)))
			err(1, "setsockopt(IPV6_RECVPKTINFO)");
#else  /* old adv. API */
		if (setsockopt(s, IPPROTO_IPV6, IPV6_PKTINFO, &opton,
		    sizeof(opton)))
			err(1, "setsockopt(IPV6_PKTINFO)");
#endif
#endif /* USE_SIN6_SCOPE_ID */
#ifdef IPV6_RECVHOPLIMIT
		if (setsockopt(s, IPPROTO_IPV6, IPV6_RECVHOPLIMIT, &opton,
		    sizeof(opton)))
			err(1, "setsockopt(IPV6_RECVHOPLIMIT)");
#else  /* old adv. API */
		if (setsockopt(s, IPPROTO_IPV6, IPV6_HOPLIMIT, &opton,
		    sizeof(opton)))
			err(1, "setsockopt(IPV6_HOPLIMIT)");
#endif
#ifdef IPV6_CHECKSUM
#ifndef SOL_RAW
#define SOL_RAW IPPROTO_IPV6
#endif
		opton = 2;
		if (setsockopt(s, SOL_RAW, IPV6_CHECKSUM, &opton,
		    sizeof(opton)))
			err(1, "setsockopt(SOL_RAW,IPV6_CHECKSUM)");
#endif
#endif

	if( ( uid = getuid() ) )
	{
		seteuid( getuid() );

	}/* IF */

	prog = argv[0];
	ident = getpid() & 0xFFFF;

	verbose_flag = 1;
	backoff_flag = 1;
	opterr = 1;

	/* get command line options */

	while( ( c = getopt( argc, argv, "gedhlmnqusaAvz:t:i:p:f:r:c:b:C:Q:B:S:I:T:" ) ) != EOF )
	{
		switch( c )
		{
		case 't':
			if( !( timeout = ( u_int )atoi( optarg ) * 100 ) )
				usage();

			break;
		
		case 'r':
			if( ( retry = ( u_int )atoi( optarg ) ) < 0 )
				usage();

			break;
		
		case 'i':
			if( !( interval = ( u_int )atoi( optarg ) * 100 ) )
				usage();

			break;

		case 'p':
			if( !( perhost_interval = ( u_int )atoi( optarg ) * 100 ) )
				usage();

			break;

		case 'c':
			if( !( count = ( u_int )atoi( optarg ) ) )
				usage();
			
			count_flag = 1;
			break;
		
		case 'C':
			if( !( count = ( u_int )atoi( optarg ) ) )
				usage();
			
			count_flag = 1;
			report_all_rtts_flag = 1;
			break;

		case 'b':
			if( !( ping_data_size = ( u_int )atoi( optarg ) ) )
				usage();
			
			break;

		case 'h':
			usage();
			break;

		case 'q':
			verbose_flag = 0;
			quiet_flag = 1;
			break;

		case 'Q':
			verbose_flag = 0;
			quiet_flag = 1;
			if( !( report_interval = ( u_int )atoi( optarg ) * 100000 ) )
				usage();
			
			break;

		case 'e':
			elapsed_flag = 1;
			break;

		case 'm':
			multif_flag = 1;
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
				usage();
			
			break;

		case 's':
			stats_flag = 1;
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

#if defined( DEBUG ) || defined( _DEBUG )
		case 'z':
			if( ! ( debugging = ( u_int )atoi( optarg ) ) )
				usage();
			
			break;
#endif /* DEBUG || _DEBUG */

		case 'v':
			printf( "%s: Version %s $Date: %s $\n", argv[0], VERSION, REV_DATE );
			printf( "%s: comments to %s\n", argv[0], EMAIL );
			exit( 0 );

		case 'f': 
#ifdef ENABLE_F_OPTION
			filename = optarg;
			generate_flag = 0;
			break;
#else
			if( getuid() )
			{
				printf( "%s: this option can only be used by root.\n", argv[0] );
				printf( "%s: fping will read from stdin by default.\n", argv[0] );
				exit( 3 );

			}/* IF */
			else
			{
				filename = optarg;
				generate_flag = 0;

			}/* ELSE */

			break;
#endif /* ENABLE_F_OPTION */

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
				usage();
			src_addr_present = 1;
			break;

		case 'I':
#ifdef SO_BINDTODEVICE
		        if (setsockopt(s, SOL_SOCKET, SO_BINDTODEVICE, optarg,
		                   strlen(optarg)))
			        err(1, "setsockopt(AF_INET, SO_BINDTODEVICE)");
#else
			fprintf( stderr,
				 "Warning: SO_BINDTODEVICE not supported, argument -I %s ignored\n",
				 optarg );
#endif
		        break;

		case 'T':
		        if ( ! ( select_time = ( u_int )atoi( optarg ) * 100 ) )
				usage();
		        break;

		default:
			usage();
			break;

		}/* SWITCH */
	}/* WHILE */

	/* validate various option settings */

	if( unreachable_flag && alive_flag )
	{
		fprintf( stderr, "%s: specify only one of a, u\n", argv[0] );
		usage();

	}/* IF */

	if( count_flag && loop_flag )
	{
		fprintf( stderr, "%s: specify only one of c, l\n", argv[0] );
		usage();
	
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
		usage();

	}/* IF */
	
	if( ( ping_data_size > MAX_PING_DATA ) || ( ping_data_size < MIN_PING_DATA ) )
	{
		fprintf( stderr, "%s: data size %u not valid, must be between %u and %u\n",
			prog, ping_data_size, MIN_PING_DATA, MAX_PING_DATA );
		usage();
	
	}/* IF */
	
	if( ( backoff > MAX_BACKOFF_FACTOR ) || ( backoff < MIN_BACKOFF_FACTOR ) )
	{
		fprintf( stderr, "%s: backoff factor %.1f not valid, must be between %.1f and %.1f\n",
			prog, backoff, MIN_BACKOFF_FACTOR, MAX_BACKOFF_FACTOR );
		usage();
	
	}/* IF */

	if( count > MAX_COUNT )
	{
		fprintf( stderr, "%s: count %u not valid, must be less than %u\n",
			prog, count, MAX_COUNT );
	    usage();
	
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

	}/* IF */
#endif /* DEBUG || _DEBUG */

	/* handle host names supplied on command line or in a file */
	/* if the generate_flag is on, then generate the IP list */

	argv = &argv[optind];

	/* cover allowable conditions */

	/* file and generate are mutually exclusive */
	/* file and command line are mutually exclusive */
	/* generate requires command line parameters beyond the switches */
	if( ( *argv && filename ) || ( filename && generate_flag ) || ( generate_flag && !*argv ) )
		usage();

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
		char *p;
		
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
			
			p = cpystr( host );
			add_name( p );
		
		}/* WHILE */
		
		fclose( ping_file );

	}/* ELSE IF */
	else if( *argv && generate_flag )
	{
		char* pStart;
		char* pEnd;
		char* pCopy;
		char* pTemp;

		struct in_addr sStart;
		struct in_addr sEnd;
		int iBits;
		int iBitpos;
		int iMask = 1;
		int failed = 0;
		unsigned long uTemp;

		/* two possible forms are allowed here */

		pStart = *argv;
		argv++;

		/* IP mask is specified */
		if( !*argv )
		{
			pCopy = ( char* )malloc( sizeof( char ) * strlen( pStart ) + 1 );
			if( pCopy )
			{
				/* make a copy of the arg, so we don't damage the original */
				strcpy( pCopy, pStart );

				/* look for token '/' */
				if( strtok( pCopy, "/" ) != NULL )
				{
					/* if no token was found, the string should be unaltered */
					if( strcmp( pCopy, pStart ) != 0 )
					{
						/* convert it to the starting address */
						if( inet_addr( pCopy ) != INADDR_NONE )
						{
							sStart.s_addr = inet_addr( pCopy );

							/* now find the bitmask */
							pTemp = ( char* )pStart + strlen( pCopy ) + 1;
	
							/* get the bits */
							iBits = 32 - atoi( pTemp );

							if( ( iBits < 32 ) && ( iBits >= 0 ) )
							{
								/* now make the end address */
								for( iBitpos = 0; iBitpos < iBits; iBitpos++ )
									iMask = iMask | 1 << iBitpos;

								sEnd.s_addr = sStart.s_addr | ntohl( iMask );

							}/* IF */
							else
								failed = 1;

						}/* IF */
						else
							failed = 1;

					}/* IF */
					else
						failed = 1;

				}/* IF */
				else
					failed = 1;

				free( pCopy );

				if( failed == 1 )
					usage();

			}/* IF */
			else
				crash_and_burn( "Cannot malloc copy of input string" );

		}/* IF */
		else
		{
			/* IP start and end points are specified */
			pEnd = *argv;

			/* parameters should be start and end ranges */
			if( ( inet_addr( pStart ) != INADDR_NONE ) && ( inet_addr( pEnd ) != INADDR_NONE ) )
			{
				sStart.s_addr = inet_addr( pStart );
				sEnd.s_addr = inet_addr( pEnd );

			}/* IF */
			else
				usage();

		}/* ELSE */

		/* ensure that the end point is greater or equal to the start */
		if( htonl( sEnd.s_addr ) >= htonl( sStart.s_addr ) )
		{
			/* start and end points should be determined, so generate list */
			unsigned int uiDiff;
			struct in_addr sTemp;
			int iCount;

			uiDiff = htonl( sEnd.s_addr ) - htonl( sStart.s_addr ) + 1;

			for( iCount = 0; iCount < uiDiff; iCount++ )
			{
				sTemp.s_addr = sStart.s_addr + ntohl( iCount );
				pTemp = cpystr( inet_ntoa( sTemp ) );
				add_name( pTemp );

			}/* FOR */
		}/* IF */
		else
			usage();

	}/* ELSE IF */
	else
		usage();
	
	if( !num_hosts )
		exit( 2 );

	/* set the source address */

	if( src_addr_present )
	{
		memset( &sa, 0, sizeof( sa ) );
#ifndef IPV6
		sa.sin_family = AF_INET;
		sa.sin_addr = src_addr;
#else
		sa.sin6_family = AF_INET6;
		sa.sin6_addr = src_addr;
#endif
		if ( bind( s, (struct sockaddr *)&sa, sizeof( sa ) ) < 0 )
			errno_crash_and_burn( "cannot bind source address" );
	}

	/* allocate array to hold outstanding ping requests */

	table = ( HOST_ENTRY** )malloc( sizeof( HOST_ENTRY* ) * num_hosts );
	if( !table )
		crash_and_burn( "Can't malloc array of hosts" );

	cursor = rrlist;

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
		
		cursor=cursor->next;

	}/* FOR */

	ping_pkt_size = ping_data_size + SIZE_ICMP_HDR;
	
	signal( SIGINT, finish );
	
	gettimeofday( &start_time, &tz );
	current_time = start_time;

	if( report_interval )
		last_report_time = start_time;

	last_send_time.tv_sec = current_time.tv_sec - 10000;

#if defined( DEBUG ) || defined( _DEBUG )
	if( randomly_lose_flag ) 
		srandom( start_time.tv_usec );
#endif /* DEBUG || _DEBUG */

	cursor = rrlist;
	advance = 0;

	/* main loop */

	while( num_jobs )
	{
		if( num_pingsent )
			while( wait_for_reply() );  /* call wfr until we timeout */
			
		if( cursor && advance )
			cursor = cursor->next;
		
		gettimeofday( &current_time, &tz );
		lt = timeval_diff( &current_time, &last_send_time );
		ht = timeval_diff( &current_time, &cursor->last_send_time );

		if( report_interval && ( loop_flag || count_flag ) &&
			( timeval_diff ( &current_time, &last_report_time )	> report_interval ) )
		{
			print_per_system_splits();
			gettimeofday( &current_time, &tz );
			lt = timeval_diff( &current_time, &last_send_time );
			ht = timeval_diff( &current_time, &cursor->last_send_time );
			last_report_time = current_time;
		
		}/* IF */
		
		advance = 1;

#if defined( DEBUG ) || defined( _DEBUG )
		if( trace_flag )
		{
			printf(
				"main loop:\n  [%s, wait/run/sent/recv/timeout = %u/%u/%u/%u/%u], jobs/lt/ht = %u/%u/%u\n",
				cursor->host, cursor->waiting, cursor->running, cursor->num_sent, 
				cursor->num_recv, cursor->timeout, num_jobs, lt, ht );

		}/* IF */
#endif /* DEBUG || _DEBUG */

		/* if it's OK to send while counting or looping or starting */
		if( ( lt > interval ) && ( ht > perhost_interval ) )
		{
			/* send if starting or looping */
			if( ( cursor->num_sent == 0 ) || loop_flag )
			{
				send_ping( s, cursor );
				continue;
			
			}/* IF */
		
			/* send if counting and count not exceeded */
			if( count_flag )
			{
				if( cursor->num_sent < count )
				{
					send_ping( s, cursor );
					continue;
				
				}/* IF */
			}/* IF */
		}/* IF */

		/* is-it-alive mode, and timeout exceeded while waiting for a reply */
		/*   and we haven't exceeded our retries                            */
		if( ( lt > interval ) && !count_flag && !loop_flag && !cursor->num_recv &&
			( ht > cursor->timeout ) && ( cursor->waiting < retry + 1 ) )
		{
#if defined( DEBUG ) || defined( _DEBUG )
			if( trace_flag ) 
				printf( "main loop: timeout for %s\n", cursor->host );
#endif /* DEBUG || _DEBUG */

			num_timeout++;

			/* try again */
			if( backoff_flag )
				cursor->timeout *= backoff;

			send_ping( s, cursor );
			continue;
		
		}/* IF */

		/* didn't send, can we remove? */

#if defined( DEBUG ) || defined( _DEBUG )
		if( trace_flag )
			printf( "main loop: didn't send to %s\n", cursor->host );
#endif /* DEBUG || _DEBUG */
    
		/* never remove if looping */
		if( loop_flag )
			continue;

		/* remove if counting and count exceeded */
		/* but allow time for the last one to come in */
		if( count_flag )
		{
			if( ( cursor->num_sent >= count ) && ( cursor->num_recv >= count || ht > cursor->timeout ) )
			{
				remove_job( cursor );
				continue;
			
			}/* IF */
		}/* IF */
		else
		{
			/* normal mode, and we got one */
			if( cursor->num_recv )
			{
				remove_job( cursor );
				continue;
			
			}/* IF */
			
			/* normal mode, and timeout exceeded while waiting for a reply */
			/* and we've run out of retries, so node is unreachable */
			if( ( ht > cursor->timeout ) && ( cursor->waiting >= retry + 1 ) )
			{
#if defined( DEBUG ) || defined( _DEBUG )
				if( trace_flag ) 
					printf( "main loop: timeout for %s\n", cursor->host );
#endif /* DEBUG || _DEBUG */

				num_timeout++;
				remove_job( cursor );
				continue;
			
			}/* IF */
		}/* ELSE */
		
		/* could send to this host, so keep considering it */
		if( ht > interval )
			advance = 0;

	}/* WHILE */
	
	finish();

	return 0;
} /* main() */


/************************************************************

  Function: finish

*************************************************************

  Inputs:  void (none)

  Description:
  
  Main program clean up and exit point

************************************************************/

#ifdef _NO_PROTO
void finish()
#else
void finish()
#endif /* _NO_PROTO */
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

#ifdef _NO_PROTO
void print_per_system_stats()
#else
void print_per_system_stats( void )
#endif /* _NO_PROTO */
{
	int i, j, k, avg;
	HOST_ENTRY *h;
	char *buf;
	int bufsize;
	int resp;

	bufsize = max_hostname_len + 1;
	buf = ( char* )malloc( bufsize );

	if( !buf )
		crash_and_burn( "can't malloc print buf" );

	memset( buf, 0, bufsize );

	fflush( stdout );

	if( verbose_flag || per_recv_flag )
		fprintf( stderr, "\n" );

	for( i = 0; i < num_hosts; i++ )
	{
		h = table[i];
		fprintf( stderr, "%s%s :", h->host, h->pad );

		if( report_all_rtts_flag )
		{
			for( j = 0; j < h->num_sent; j++ )
			{
				if( ( resp = h->resp_times[j] ) >= 0 )
					fprintf( stderr, " %d.%02d", resp / 100, resp % 100 );
				else
					fprintf( stderr, " -" );

			}/* FOR */
		  
			fprintf( stderr, "\n" );
	  
		}/* IF */
		else
		{
			if( h->num_recv <= h->num_sent )
			{
				fprintf( stderr, " xmt/rcv/%%loss = %d/%d/%d%%",
					h->num_sent, h->num_recv, h->num_sent > 0 ?
					( ( h->num_sent - h->num_recv ) * 100 ) / h->num_sent : 0 );

			}/* IF */
			else
			{
				fprintf( stderr, " xmt/rcv/%%return = %d/%d/%d%%",
					h->num_sent, h->num_recv,
					( ( h->num_recv * 100 ) / h->num_sent ) );
		  
			}/* ELSE */
		  
			if( h->num_recv )
			{
				avg = h->total_time / h->num_recv;
				fprintf( stderr, ", min/avg/max = %s", sprint_tm( h->min_reply ) );
				fprintf( stderr, "/%s", sprint_tm( avg ) );
				fprintf( stderr, "/%s", sprint_tm( h->max_reply ) );
		  
			}/* IF */
		  
			fprintf(stderr, "\n");

		}/* ELSE */

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

			}/* FOR */
		}/* IF */
#endif /* DEBUG || _DEBUG */
	}/* FOR */
  
	free( buf );

} /* print_per_system_stats() */


/************************************************************

  Function: print_per_system_splits

*************************************************************

  Inputs:  void (none)

  Description:
  

************************************************************/

#ifdef _NO_PROTO
void print_per_system_splits()
#else
void print_per_system_splits( void )
#endif /* _NO_PROTO */
{
	int i, j, k, avg;
	HOST_ENTRY *h;
	char *buf;
	int bufsize;
	int resp;
	struct tm *curr_tm;

	bufsize = max_hostname_len + 1;
	buf = ( char* )malloc( bufsize );
	if( !buf )
		crash_and_burn( "can't malloc print buf" );

	memset( buf, 0, bufsize );

	fflush( stdout );

	if( verbose_flag || per_recv_flag )
		fprintf( stderr, "\n" );

	curr_tm = localtime( ( time_t* )&current_time.tv_sec );
	fprintf( stderr, "[%2.2d:%2.2d:%2.2d]\n", curr_tm->tm_hour,
		curr_tm->tm_min, curr_tm->tm_sec );

	for( i = 0; i < num_hosts; i++ )
	{
		h = table[i];
		fprintf( stderr, "%s%s :", h->host, h->pad );

		if( h->num_recv_i <= h->num_sent_i )
		{
			fprintf( stderr, " xmt/rcv/%%loss = %d/%d/%d%%",
				h->num_sent_i, h->num_recv_i, h->num_sent_i > 0 ?
				( ( h->num_sent_i - h->num_recv_i ) * 100 ) / h->num_sent_i : 0 );

		}/* IF */
		else
		{
			fprintf( stderr, " xmt/rcv/%%return = %d/%d/%d%%",
				h->num_sent_i, h->num_recv_i, h->num_sent_i > 0 ?
				( ( h->num_recv_i * 100 ) / h->num_sent_i ) : 0 );

		}/* ELSE */

		if( h->num_recv_i )
		{
			avg = h->total_time_i / h->num_recv_i;
			fprintf( stderr, ", min/avg/max = %s", sprint_tm( h->min_reply_i ) );
			fprintf( stderr, "/%s", sprint_tm( avg ) );
			fprintf( stderr, "/%s", sprint_tm( h->max_reply_i ) );
		
		}/* IF */
		
		fprintf( stderr, "\n" );
		h->num_sent_i = h->num_recv_i = h->max_reply_i =
			h->min_reply_i = h->total_time_i = 0;
	
	}/* FOR */

	free( buf );

} /* print_per_system_splits() */


/************************************************************

  Function: print_global_stats

*************************************************************

  Inputs:  void (none)

  Description:
  

************************************************************/

#ifdef _NO_PROTO
void print_global_stats()
#else
void print_global_stats( void )
#endif /* _NO_PROTO */
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

#ifdef _NO_PROTO
void send_ping( s, h )
int s; HOST_ENTRY *h;
#else
void send_ping( int s, HOST_ENTRY *h )
#endif /* _NO_PROTO */
{
	char *buffer;
	FPING_ICMPHDR *icp;
	PING_DATA *pdp;
	int n;

	buffer = ( char* )malloc( ( size_t )ping_pkt_size );
	if( !buffer )
		crash_and_burn( "can't malloc ping packet" );
	
	memset( buffer, 0, ping_pkt_size * sizeof( char ) );
	icp = ( FPING_ICMPHDR* )buffer;

	gettimeofday( &h->last_send_time, &tz );
	int myseq = h->num_sent * num_hosts + h->i;
	max_seq_sent = myseq > max_seq_sent ? myseq : max_seq_sent;

#ifndef IPV6
	icp->icmp_type = ICMP_ECHO;
	icp->icmp_code = 0;
	icp->icmp_cksum = 0;
	icp->icmp_seq = htons(myseq);
	icp->icmp_id = htons(ident);

	pdp = ( PING_DATA* )( buffer + SIZE_ICMP_HDR );
	pdp->ping_ts = h->last_send_time;
	pdp->ping_count = h->num_sent;

	icp->icmp_cksum = in_cksum( ( u_short* )icp, ping_pkt_size );
#else
	icp->icmp6_type = ICMP6_ECHO_REQUEST;
	icp->icmp6_code = 0;
	icp->icmp6_seq = htons(myseq);
	icp->icmp6_id = htons(ident);

	pdp = ( PING_DATA* )( buffer + SIZE_ICMP_HDR );
	pdp->ping_ts = h->last_send_time;
	pdp->ping_count = h->num_sent;

	icp->icmp6_cksum = 0;	// The IPv6 stack calculates the checksum for us...
#endif
#if defined(DEBUG) || defined(_DEBUG)
	if( trace_flag )
		printf( "sending [%d] to %s\n", h->num_sent, h->host );
#endif /* DEBUG || _DEBUG */

	n = sendto( s, buffer, ping_pkt_size, 0,
		( struct sockaddr* )&h->saddr, sizeof( FPING_SOCKADDR ) );

	if( n < 0 || n != ping_pkt_size )
	{
		if( verbose_flag || unreachable_flag )
		{
			printf( "%s", h->host );
			if( verbose_flag )
				printf( " error while sending ping: %s\n", strerror( errno ) );
			
			printf( "\n" );

		}/* IF */
		
		num_unreachable++;
		remove_job( h ); 

	}/* IF */
	else
	{
		/* mark this trial as outstanding */
		if( !loop_flag )
			h->resp_times[h->num_sent] = RESP_WAITING;

#if defined( DEBUG ) || defined( _DEBUG )
		if( sent_times_flag )
			h->sent_times[h->num_sent] = timeval_diff( &h->last_send_time, &start_time );
#endif /* DEBUG || _DEBUG */

		h->num_sent++;
		h->num_sent_i++;
		h->waiting++;
		num_pingsent++;
		last_send_time = h->last_send_time;

	}/* ELSE */
	
	free( buffer );

} /* send_ping() */


/************************************************************

  Function: wait_for_reply

*************************************************************

  Inputs:  void (none)

  Returns:  int

  Description:
  

************************************************************/

#ifdef _NO_PROTO
int wait_for_reply()
#else
int wait_for_reply( void )
#endif
{
	int result;
	static char buffer[4096];
	FPING_SOCKADDR response_addr;
	struct ip *ip;
	int hlen = 0;
	FPING_ICMPHDR *icp;
	int n, avg;
	HOST_ENTRY *h;
	long this_reply;
	int this_count;
	struct timeval sent_time;

	result = recvfrom_wto( s, buffer, sizeof(buffer), &response_addr, select_time );

	if( result < 0 )
		return 0;	/* timeout */
  
#if defined( DEBUG ) || defined( _DEBUG )
	if( randomly_lose_flag )
	{
		if( ( random() & 0x07 ) <= lose_factor )
			return 0;

	}/* IF */
#endif /* DEBUG || _DEBUG */

	ip = ( struct ip* )buffer;
#ifndef IPV6
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
#ifndef IPV6
			printf( "received packet too short for ICMP (%d bytes from %s)\n", result,
				inet_ntoa( response_addr.sin_addr ) );
#else			
			char buf[INET6_ADDRSTRLEN];
			inet_ntop(response_addr.sin6_family, &response_addr.sin6_addr, buf, INET6_ADDRSTRLEN);
			printf( "received packet too short for ICMP (%d bytes from %s)\n", result, buf);
#endif
		}
		return( 1 ); /* too short */ 
	}/* IF */

	icp = ( FPING_ICMPHDR* )( buffer + hlen );
#ifndef IPV6
	if( icp->icmp_type != ICMP_ECHOREPLY )
#else
	if( icp->icmp6_type != ICMP6_ECHO_REPLY )
#endif
	{
		/* handle some problem */
		if( handle_random_icmp( icp, result, &response_addr ) )
			num_othericmprcvd++;
		return 1;
	}/* IF */

#ifndef IPV6
	if( ntohs(icp->icmp_id) != ident )
#else
	if( ntohs(icp->icmp6_id) != ident )
#endif
		return 1; /* packet received, but not the one we are looking for! */

	num_pingreceived++;

#ifndef IPV6
	if( ntohs(icp->icmp_seq)  > max_seq_sent )
#else
	if( ntohs(icp->icmp6_seq) > max_seq_sent )
#endif
		return( 1 ); /* packet received, don't worry about it anymore */

#ifndef IPV6
	n = ntohs(icp->icmp_seq) % num_hosts;
#else
	n = ntohs(icp->icmp6_seq) % num_hosts;
#endif
	h = table[n];

	/* received ping is cool, so process it */
	gettimeofday( &current_time, &tz );
	h->waiting = 0;
	h->timeout = timeout;
	h->num_recv++;
	h->num_recv_i++;

#ifndef IPV6
	memcpy( &sent_time, icp->icmp_data + offsetof( PING_DATA, ping_ts ), sizeof( sent_time ) );
	memcpy( &this_count, icp->icmp_data, sizeof( this_count ) );
#else
	memcpy( &sent_time, ((char *)icp->icmp6_data32)+4+offsetof(PING_DATA, ping_ts), sizeof( sent_time ) );
	memcpy( &this_count, ((char *)icp->icmp6_data32)+4, sizeof( this_count ) );
#endif

#if defined( DEBUG ) || defined( _DEBUG )
	if( trace_flag ) 
		printf( "received [%d] from %s\n", this_count, h->host );
#endif /* DEBUG || _DEBUG */

	this_reply = timeval_diff( &current_time, &sent_time );
	if( this_reply > max_reply ) max_reply = this_reply;
	if( this_reply < min_reply ) min_reply = this_reply;
	if( this_reply > h->max_reply ) h->max_reply = this_reply;
	if( this_reply < h->min_reply ) h->min_reply = this_reply;
	if( this_reply > h->max_reply_i ) h->max_reply_i = this_reply;
	if( this_reply < h->min_reply_i ) h->min_reply_i = this_reply;
	sum_replies += this_reply;
	h->total_time += this_reply;
	h->total_time_i += this_reply;
	total_replies++;
	
	/* note reply time in array, probably */
	if( !loop_flag )
	{
		if( ( this_count >= 0 ) && ( this_count < trials ) )
		{
			if( h->resp_times[this_count] != RESP_WAITING )
			{
				if( !per_recv_flag )
				{
					fprintf( stderr, "%s : duplicate for [%d], %d bytes, %s ms",
						h->host, this_count, result, sprint_tm( this_reply ) );
#ifndef IPV6
					if( response_addr.sin_addr.s_addr != h->saddr.sin_addr.s_addr )
						fprintf( stderr, " [<- %s]", inet_ntoa( response_addr.sin_addr ) );
#else
					if(memcmp(&response_addr.sin6_addr, &h->saddr.sin6_addr, sizeof(response_addr.sin6_addr)))
					{
						char buf[INET6_ADDRSTRLEN];
						inet_ntop(response_addr.sin6_family, &response_addr.sin6_addr, buf, INET6_ADDRSTRLEN);

						fprintf( stderr, " [<- %s]", buf);
					}
#endif	  
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
#ifndef IPV6
			if( response_addr.sin_addr.s_addr != h->saddr.sin_addr.s_addr )
				printf( " [<- %s]", inet_ntoa( response_addr.sin_addr ) );
#else
      	if(memcmp(&response_addr.sin6_addr, &h->saddr.sin6_addr, sizeof(response_addr.sin6_addr)))
			{
				char buf[INET6_ADDRSTRLEN];
				inet_ntop(response_addr.sin6_family, &response_addr.sin6_addr, buf, INET6_ADDRSTRLEN);
				fprintf( stderr, " [<- %s]", buf);
			}
#endif
			printf( "\n" );
		
		}/* IF */
	}/* IF */

	if( per_recv_flag )
	{
		avg = h->total_time / h->num_recv;
		printf( "%s%s : [%d], %d bytes, %s ms",
			h->host, h->pad, this_count, result, sprint_tm( this_reply ) );
		printf( " (%s avg, ", sprint_tm( avg ) );
    
		if( h->num_recv <= h->num_sent )
		{
			printf( "%d%% loss)",
				( ( h->num_sent - h->num_recv ) * 100 ) / h->num_sent );

		}/* IF */
		else
		{
			printf( "%d%% return)",
				( h->num_recv * 100 ) / h->num_sent );
		
		}/* ELSE */
#ifndef IPV6
		if( response_addr.sin_addr.s_addr != h->saddr.sin_addr.s_addr )
			printf( " [<- %s]", inet_ntoa( response_addr.sin_addr ) );
#else
   	if(memcmp(&response_addr.sin6_addr, &h->saddr.sin6_addr, sizeof(response_addr.sin6_addr)))
		{
			char buf[INET6_ADDRSTRLEN];
			inet_ntop(response_addr.sin6_family, &response_addr.sin6_addr, buf, INET6_ADDRSTRLEN);
			fprintf( stderr, " [<- %s]", buf);
		}
#endif
		
		printf( "\n" );
	
	}/* IF */
	
	fflush( stdout );
	return num_jobs;

} /* wait_for_reply() */

/************************************************************

  Function: handle_random_icmp

*************************************************************

  Inputs:  FPING_ICMPHDR *p, int psize, FPING_SOCKADDR *addr

  Returns:  int

  Description:
  

************************************************************/

#ifdef _NO_PROTO
int handle_random_icmp( p, psize, addr )
     FPING_ICMPHDR *p;
     int psize;
     FPING_SOCKADDR *addr;
#else
int handle_random_icmp( FPING_ICMPHDR *p, int psize, FPING_SOCKADDR *addr )
#endif /* _NO_PROTO */
{
	FPING_ICMPHDR *sent_icmp;
	struct ip *sent_ip;
	u_char *c;
	HOST_ENTRY *h;
#ifdef IPV6
	char addr_ascii[INET6_ADDRSTRLEN];
	inet_ntop(addr->sin6_family, &addr->sin6_addr, addr_ascii, INET6_ADDRSTRLEN);
#endif

	c = ( u_char* )p;
#ifndef IPV6
	switch( p->icmp_type )
#else
	switch( p->icmp6_type )
#endif
	{
	case ICMP_UNREACH:
		sent_icmp = ( FPING_ICMPHDR* )( c + 28 );
		
#ifndef IPV6
		sent_icmp = ( struct icmp* )( c + 28 );
		
		if( ( sent_icmp->icmp_type == ICMP_ECHO ) &&
			( ntohs(sent_icmp->icmp_id) == ident ) &&
			( ntohs(sent_icmp->icmp_seq) <= ( n_short )max_seq_sent ) )
		{
			/* this is a response to a ping we sent */
			h = table[ntohs(sent_icmp->icmp_seq) % num_hosts];
			
			if( p->icmp_code > ICMP_UNREACH_MAXTYPE )
			{
				fprintf( stderr, "ICMP Unreachable (Invalid Code) from %s for ICMP Echo sent to %s",
					inet_ntoa( addr->sin_addr ), h->host );

#else
		if( ( sent_icmp->icmp6_type == ICMP_ECHO ) &&
			( ntohs(sent_icmp->icmp6_id) == ident ) &&
			( ntohs(sent_icmp->icmp6_seq) <= ( n_short )max_seq_sent ) )
		{
			/* this is a response to a ping we sent */
			h = table[ntohs(sent_icmp->icmp6_seq) % num_hosts];
			
			if( p->icmp6_code > ICMP_UNREACH_MAXTYPE )
			{
				fprintf( stderr, "ICMP Unreachable (Invalid Code) from %s for ICMP Echo sent to %s",
					addr_ascii, h->host );
#endif
			}/* IF */
			else
			{
				fprintf( stderr, "%s from %s for ICMP Echo sent to %s",
#ifndef IPV6
					icmp_unreach_str[p->icmp_code], inet_ntoa( addr->sin_addr ), h->host );
#else
					icmp_unreach_str[p->icmp6_code], addr_ascii, h->host );
#endif
			
			}/* ELSE */

			if( inet_addr( h->host ) == -1 )
#ifndef IPV6
				fprintf( stderr, " (%s)", inet_ntoa( h->saddr.sin_addr ) );
#else
				fprintf( stderr, " (%s)", addr_ascii);
#endif
			
			fprintf( stderr, "\n" );
		
		}/* IF */

		return 1;

	case ICMP_SOURCEQUENCH:
	case ICMP_REDIRECT:
	case ICMP_TIMXCEED:
	case ICMP_PARAMPROB:
		sent_icmp = ( FPING_ICMPHDR* )( c + 28 );
#ifndef IPV6
		if( ( sent_icmp->icmp_type == ICMP_ECHO ) &&
			( ntohs(sent_icmp->icmp_id) == ident ) &&
			( ntohs(sent_icmp->icmp_seq) <= ( n_short )max_seq_sent ) )
		{
			/* this is a response to a ping we sent */
			h = table[ntohs(sent_icmp->icmp_seq) % num_hosts];
			fprintf( stderr, "%s from %s for ICMP Echo sent to %s",
				icmp_type_str[p->icmp_type], inet_ntoa( addr->sin_addr ), h->host );
      
			if( inet_addr( h->host ) == -1 )
				fprintf( stderr, " (%s)", inet_ntoa( h->saddr.sin_addr ) );
#else
		if( ( sent_icmp->icmp6_type == ICMP_ECHO ) &&
			( ntohs(sent_icmp->icmp6_id) == ident ) &&
			( ntohs(sent_icmp->icmp6_seq) <= ( n_short )max_seq_sent ) )
		{
			/* this is a response to a ping we sent */
			h = table[ntohs(sent_icmp->icmp6_seq) % num_hosts];
			fprintf( stderr, "%s from %s for ICMP Echo sent to %s",
				icmp_type_str[p->icmp6_type], addr_ascii, h->host );
      
			if( inet_addr( h->host ) == -1 )
				fprintf( stderr, " (%s)", addr_ascii );
#endif

			fprintf( stderr, "\n" );
		
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

  Function: in_cksum

*************************************************************

  Inputs:  u_short *p, int n

  Returns:  int

  Description:

  Checksum routine for Internet Protocol family headers (C Version)
  From ping examples in W.Richard Stevens "UNIX NETWORK PROGRAMMING" book.

************************************************************/

#ifdef _NO_PROTO
int in_cksum( p, n )
u_short *p; int n;
#else
int in_cksum( u_short *p, int n )
#endif /* _NO_PROTO */
{
	register u_short answer;
	register long sum = 0;
	u_short odd_byte = 0;

	while( n > 1 )
	{
		sum += *p++;
		n -= 2;
	
	}/* WHILE */


	/* mop up an odd byte, if necessary */
	if( n == 1 )
	{
		*( u_char* )( &odd_byte ) = *( u_char* )p;
		sum += odd_byte;
	
	}/* IF */

	sum = ( sum >> 16 ) + ( sum & 0xffff );	/* add hi 16 to low 16 */
	sum += ( sum >> 16 );					/* add carry */
	answer = ~sum;							/* ones-complement, truncate*/
	
	return ( answer );

} /* in_cksum() */


/************************************************************

  Function: add_name

*************************************************************

  Inputs:  char* name

  Description:

  process input name for addition to target list
  name can turn into multiple targets via multiple interfaces (-m)
  or via NIS groups

************************************************************/

#ifdef _NO_PROTO
void add_name( name )
char *name;
#else
void add_name( char *name )
#endif /* _NO_PROTO */
{
#ifndef IPV6
	struct hostent *host_ent;
	u_int ipaddress;
	struct in_addr *ipa = ( struct in_addr* )&ipaddress;
	struct in_addr *host_add;
	char *nm;
	int i = 0;

	if( ( ipaddress = inet_addr( name ) ) != -1 )
	{
		/* input name is an IP addr, go with it */
		if( name_flag )
		{
			if( addr_flag )
				add_addr( name, na_cat( get_host_by_address( *ipa ), *ipa ), *ipa );
			else
			{
				nm = cpystr( get_host_by_address( *ipa ) );
				add_addr( name, nm, *ipa );

			}/* ELSE */
		}/* IF */
		else
			add_addr( name, name, *ipa );
		
		return;
	
	}/* IF */

	/* input name is not an IP addr, maybe it's a host name */
	host_ent = gethostbyname( name ); 
	if( host_ent == NULL )
	{ 
		if( h_errno == TRY_AGAIN )
		{ 
			u_sleep( DNS_TIMEOUT ); 
			host_ent = gethostbyname( name );

		}/* IF */

		if( host_ent == NULL )
		{
#ifdef NIS_GROUPS

			/* maybe it's the name of a NIS netgroup */
			char *machine, *user_ignored, *domain_ignored;
			setnetgrent( name );
			if( getnetgrent( &machine, &user_ignored, &domain_ignored ) == 0 )
			{
				endnetgrent();
				if( !quiet_flag )
					fprintf( stderr, "%s address not found\n", name );
				
				num_noaddress++;
				return;
			
			}/* IF */
			else
				add_name( cpystr( machine ) );

			while( getnetgrent( &machine, &user_ignored, &domain_ignored ) )
				add_name( cpystr( machine ) );
      
			endnetgrent();
			return;
#else
			if( !quiet_flag )
				fprintf( stderr, "%s address not found\n", name );
			
			num_noaddress++;
			return ; 
#endif /* NIS_GROUPS */
		}/* IF */
	}/* IF */
  
	host_add = ( struct in_addr* )*( host_ent->h_addr_list ); 
	if( host_add == NULL )
	{ 
		if( !quiet_flag )
			fprintf( stderr, "%s has no address data\n", name );
		
		num_noaddress++;
		return; 

	}/* IF */
	else
	{
		/* it is indeed a hostname with a real address */
		while( host_add )
		{
			if( name_flag && addr_flag )
				add_addr( name, na_cat( name, *host_add ), *host_add );
			else if( addr_flag )
			{
				nm = cpystr( inet_ntoa( *host_add ) );
				add_addr( name, nm, *host_add );
			}/* ELSE IF */
			else
				add_addr( name, name, *host_add );
			
			if( !multif_flag )
				break;

			host_add = ( struct in_addr* )( host_ent->h_addr_list[++i] ); 

		}/* WHILE */
	}/* ELSE */
#else
	FPING_SOCKADDR	dst;
	struct addrinfo		*res, hints;
	int						ret_ga;
	char						*hostname;
	size_t len;

	/* getaddrinfo */
	bzero(&hints, sizeof(struct addrinfo));
	hints.ai_flags = name_flag ? AI_CANONNAME : 0;
	hints.ai_family = AF_INET6;
	hints.ai_socktype = SOCK_RAW;
	hints.ai_protocol = IPPROTO_ICMPV6;

	ret_ga = getaddrinfo(name, NULL, &hints, &res);
	if (ret_ga) {
		if(!quiet_flag)
			warnx("%s", gai_strerror(ret_ga));
		num_noaddress++;
		return; 
	}
	if (res->ai_canonname) hostname = res->ai_canonname;
	else hostname = name;
	if (!res->ai_addr) {
		if(!quiet_flag)
			warnx("getaddrinfo failed");
		num_noaddress++;
		return; 
	}
	len = res->ai_addrlen;
	if (len > sizeof(FPING_SOCKADDR)) len = sizeof(FPING_SOCKADDR);
	(void)memcpy(&dst, res->ai_addr, len);
	add_addr(name, name, &dst);
#endif
} /* add_name() */


/************************************************************

  Function: na_cat

*************************************************************

  Inputs:  char* name, struct in_addr ipaddr

  Returns:  char*

  Description:

************************************************************/

#ifdef _NO_PROTO
char *na_cat( name, ipaddr )
char *name;
struct in_addr ipaddr;
#else
char *na_cat( char *name, struct in_addr ipaddr )
#endif /* _NO_PROTO */
{
	char *nm, *as;

	as = inet_ntoa( ipaddr );
	nm = ( char* )malloc( strlen( name ) + strlen( as ) + 4 );

	if( !nm )
		crash_and_burn( "can't allocate some space for a string" );
	
	strcpy( nm, name );
	strcat( nm, " (" );
	strcat( nm, as );
	strcat( nm, ")" );

	return( nm );

} /* na_cat() */


/************************************************************

  Function: add_addr

*************************************************************

  Inputs:  char* name, char* host, struct in_addr ipaddr

  Description:

  add address to linked list of targets to be pinged
  assume memory for *name and *host is ours!!!

************************************************************/

#ifdef _NO_PROTO
void add_addr( name, host, ipaddr )
char *name;
char *host;
#ifndef IPV6
struct in_addr ipaddr;
#else
FPING_SOCKADDR *ipaddr;
#endif
#else
#ifndef IPV6
void add_addr( char *name, char *host, struct in_addr ipaddr )
#else
void add_addr( char *name, char *host, FPING_SOCKADDR *ipaddr )
#endif
#endif /* _NO_PROTO */
{
	HOST_ENTRY *p;
	int n, *i;

	p = ( HOST_ENTRY* )malloc( sizeof( HOST_ENTRY ) );
	if( !p )
		crash_and_burn( "can't allocate HOST_ENTRY" );

	memset( ( char* ) p, 0, sizeof( HOST_ENTRY ) );

	p->name = name;
	p->host = host;
#ifndef IPV6
	p->saddr.sin_family = AF_INET;
	p->saddr.sin_addr = ipaddr; 
#else
	p->saddr.sin6_family = AF_INET6;
	(void)memcpy(&p->saddr, ipaddr, sizeof(FPING_SOCKADDR));
#endif
	p->timeout = timeout;
	p->running = 1;
	p->min_reply = 10000000;

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

	if( !rrlist )
	{
		rrlist = p;
		p->next = p;
		p->prev = p;

	}/* IF */
	else
	{
		p->next = rrlist;
		p->prev = rrlist->prev;
		p->prev->next = p;
		p->next->prev = p;
	
	}/* ELSE */

	num_hosts++;

} /* add_addr() */


/************************************************************

  Function: remove_job

*************************************************************

  Inputs:  HOST_ENTRY *h

  Description:

************************************************************/

#ifdef _NO_PROTO
void remove_job( h )
HOST_ENTRY *h;
#else
void remove_job( HOST_ENTRY *h )
#endif /* _NO_PROTO */
{
#if defined( DEBUG ) || defined( _DEBUG )
	if( trace_flag )
		printf( "removing job for %s\n", h->host );
#endif /* DEBUG || _DEBUG */

	h->running = 0;
	h->waiting = 0;
	--num_jobs;

	if( num_jobs )
	{
		/* remove us from list of active jobs */
		h->prev->next = h->next;
		h->next->prev = h->prev;
		if( h==cursor )
			cursor = h->next;

	}/* IF */
	else
	{
		cursor = NULL;
		rrlist = NULL;
	
	}/* ELSE */

} /* remove_job() */


/************************************************************

  Function: get_host_by_address

*************************************************************

  Inputs:  struct in_addr in

  Returns:  char*

  Description:

************************************************************/

#ifdef _NO_PROTO
char *get_host_by_address( in )
struct in_addr in;
#else
char *get_host_by_address( struct in_addr in )
#endif /* _NO_PROTO */
{
	struct hostent *h;
#ifndef IPV6
	h = gethostbyaddr( ( char* )&in, sizeof( struct in_addr ),AF_INET );
#else
	h = gethostbyaddr( ( char* )&in, sizeof( FPING_SOCKADDR ),AF_INET6 );
#endif
	
	if( h == NULL || h->h_name == NULL )
		return inet_ntoa( in );
	else
		return ( char* )h->h_name;

} /* get_host_by_address() */


/************************************************************

  Function: cpystr

*************************************************************

  Inputs:  char* string

  Returns:  char*

  Description:

************************************************************/

#ifdef _NO_PROTO
char *cpystr( string )
char *string;
#else
char *cpystr( char *string )
#endif /* _NO_PROTO */
{
	char *dst;

	if( string )
	{
		dst = ( char* )malloc( 1 + strlen( string ) );
		if( !dst )
			crash_and_burn( "can't allocate some space for a string" );
		
		strcpy( dst, string );
		return dst;
	
	}/* IF */
	else 
		return NULL;

} /* cpystr() */


/************************************************************

  Function: crash_and_burn

*************************************************************

  Inputs:  char* message

  Description:

************************************************************/
  
#ifdef _NO_PROTO
void crash_and_burn( message )
char *message;
#else
void crash_and_burn( char *message )
#endif /* _NO_PROTO */
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

#ifdef _NO_PROTO
void errno_crash_and_burn( message )
char *message;
#else
void errno_crash_and_burn( char *message )
#endif /* _NO_PROTO */
{
	if( verbose_flag )
		fprintf( stderr, "%s: %s : %s\n", prog, message, strerror( errno ) );

	exit( 4 );

} /* errno_crash_and_burn() */


/************************************************************

  Function: timeval_diff

*************************************************************

  Inputs:  struct timeval *a, struct timeval *b

  Returns:  long

  Description:

  timeval_diff now returns result in hundredths of milliseconds
  ie, tens of microseconds                                    

************************************************************/

#ifdef _NO_PROTO
long timeval_diff( a, b )
struct timeval *a, *b;
#else
long timeval_diff( struct timeval *a, struct timeval *b )
#endif /* _NO_PROTO */
{
	double temp;

	temp = ( ( ( a->tv_sec * 1000000 ) + a->tv_usec ) -
		( ( b->tv_sec * 1000000 ) + b->tv_usec ) ) / 10;

	return ( long )temp;

} /* timeval_diff() */

/************************************************************

  Function: sprint_tm

*************************************************************

  Inputs:  int t

  Returns:  char*

  Description:

  render time into a string with three digits of precision
  input is in tens of microseconds

************************************************************/

#ifdef _NO_PROTO
char * sprint_tm( t )
int t;
#else
char * sprint_tm( int t )
#endif /* _NO_PROTO */
{
	static char buf[10];

	/* <= 0.99 ms */
	if( t < 100 )
	{
		sprintf( buf, "0.%02d", t );
		return( buf );

	}/* IF */

	/* 1.00 - 9.99 ms */
	if( t < 1000 )
	{
		sprintf( buf, "%d.%02d", t / 100, t % 100 );
		return( buf );

	}/* IF */

	/* 10.0 - 99.9 ms */
	if( t < 10000 )
	{
		sprintf( buf, "%d.%d", t / 100, ( t % 100 ) / 10 );
		return( buf );
	
	}/* IF */
  
	/* >= 100 ms */
	sprintf( buf, "%d", t / 100 );
	return( buf );

} /* sprint_tm() */


/************************************************************

  Function: u_sleep

*************************************************************

  Inputs:  int u_sec

  Description:

************************************************************/

#ifdef _NO_PROTO
void u_sleep( u_sec )
int u_sec;
#else
void u_sleep( int u_sec )
#endif /* _NO_PROTO */
{
	int nfound, slen, n;
	struct timeval to;
	fd_set readset, writeset;

	to.tv_sec = u_sec / 1000000;
	to.tv_usec = u_sec - ( to.tv_sec * 1000000 );

	FD_ZERO( &readset );
	FD_ZERO( &writeset );
	nfound = select( 0, &readset, &writeset, NULL, &to );
	if( nfound < 0 )
		errno_crash_and_burn( "select" );

	return;

} /* u_sleep() */


/************************************************************

  Function: recvfrom_wto

*************************************************************

  Inputs:  int s, char* buf, int len, FPING_SOCKADDR *saddr, int timo

  Returns:  int

  Description:

  receive with timeout
  returns length of data read or -1 if timeout
  crash_and_burn on any other errrors

************************************************************/

#ifdef _NO_PROTO
int recvfrom_wto( s, buf, len, saddr, timo )
int s; char *buf; int len; FPING_SOCKADDR *saddr; int timo;
#else
int recvfrom_wto( int s, char *buf, int len, FPING_SOCKADDR *saddr, int timo )
#endif /* _NO_PROTO */
{
	int nfound, slen, n;
	struct timeval to;
	fd_set readset, writeset;

	to.tv_sec = timo / 100000;
	to.tv_usec = ( timo - ( to.tv_sec * 100000 ) ) * 10;

	FD_ZERO( &readset );
	FD_ZERO( &writeset );
	FD_SET( s, &readset );
	nfound = select( s + 1, &readset, &writeset, NULL, &to );
	if( nfound < 0 )
		errno_crash_and_burn( "select" );

	if( nfound == 0 )
		return -1;		/* timeout */

#ifndef IPV6
	slen = sizeof( struct sockaddr );
#else
	slen = sizeof( FPING_SOCKADDR );
#endif
	n = recvfrom( s, buf, len, 0, (struct sockaddr *)saddr, &slen );
	if( n < 0 )
		errno_crash_and_burn( "recvfrom" );
	
	return n;

} /* recvfrom_wto() */


/************************************************************

  Function: usage

*************************************************************

  Inputs:  none (void)

  Description:

************************************************************/

#ifdef _NO_PROTO
void usage()
#else
void usage( void )
#endif /* _NO_PROTO */
{
	fprintf( stderr, "\n" );
	fprintf( stderr, "Usage: %s [options] [targets...]\n", prog );
	fprintf( stderr, "   -a         show targets that are alive\n" );
	fprintf( stderr, "   -A         show targets by address\n" );
	fprintf( stderr, "   -b n       amount of ping data to send, in bytes (default %d)\n", ping_data_size );
	fprintf( stderr, "   -B f       set exponential backoff factor to f\n" );
	fprintf( stderr, "   -c n       count of pings to send to each target (default %d)\n", count );  
	fprintf( stderr, "   -C n       same as -c, report results in verbose format\n" );
	fprintf( stderr, "   -e         show elapsed time on return packets\n" );
	fprintf( stderr, "   -f file    read list of targets from a file ( - means stdin) (only if no -g specified)\n" );
	fprintf( stderr, "   -g         generate target list (only if no -f specified)\n" );
	fprintf( stderr, "                (specify the start and end IP in the target list, or supply a IP netmask)\n" );
    fprintf( stderr, "                (ex. %s -g 192.168.1.0 192.168.1.255 or %s -g 192.168.1.0/24)\n", prog, prog );
	fprintf( stderr, "   -i n       interval between sending ping packets (in millisec) (default %d)\n", interval / 100 );
	fprintf( stderr, "   -l         loop sending pings forever\n" );
	fprintf( stderr, "   -m         ping multiple interfaces on target host\n" );
	fprintf( stderr, "   -n         show targets by name (-d is equivalent)\n" );
	fprintf( stderr, "   -p n       interval between ping packets to one target (in millisec)\n" );
	fprintf( stderr, "                (in looping and counting modes, default %d)\n", perhost_interval / 100 );
	fprintf( stderr, "   -q         quiet (don't show per-target/per-ping results)\n" );
	fprintf( stderr, "   -Q n       same as -q, but show summary every n seconds\n" );
	fprintf( stderr, "   -r n       number of retries (default %d)\n", DEFAULT_RETRY );
	fprintf( stderr, "   -s         print final stats\n" );
	fprintf( stderr, "   -S addr    set source address\n" );
	fprintf( stderr, "   -t n       individual target initial timeout (in millisec) (default %d)\n", timeout / 100 );
	fprintf( stderr, "   -T n       set select timeout (default %d)\n", select_time / 100 );
	fprintf( stderr, "   -u         show targets that are unreachable\n" );
	fprintf( stderr, "   -v         show version\n" );
	fprintf( stderr, "   targets    list of targets to check (if no -f specified)\n" );
	fprintf( stderr, "\n");
	exit( 3 );

} /* usage() */
