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

#include "fping.h"
#include "config.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>

int open_ping_socket_ipv4()
{
    struct protoent *proto;
    int s;

    /* confirm that ICMP is available on this machine */
    if( ( proto = getprotobyname( "icmp" ) ) == NULL )
        crash_and_burn( "icmp: unknown protocol" );

    /* create raw socket for ICMP calls (ping) */
    s = socket( AF_INET, SOCK_RAW, proto->p_proto );
    if( s < 0 ) {
        /* try non-privileged icmp (works on Mac OSX without privileges, for example) */
        s = socket( AF_INET, SOCK_DGRAM, proto->p_proto );
        if( s < 0 ) {
            errno_crash_and_burn( "can't create socket (must run as root?)" );
        }
    }

    return s;
}

void socket_set_src_addr_ipv4(int s, FPING_INADDR src_addr)
{
    struct sockaddr_in sa;
    memset( &sa, 0, sizeof( sa ) );
    sa.sin_family = AF_INET;
    sa.sin_addr = src_addr;

    if ( bind( s, (struct sockaddr *)&sa, sizeof( sa ) ) < 0 )
        errno_crash_and_burn( "cannot bind source address" );
}
