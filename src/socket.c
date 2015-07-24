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

int open_ping_socket_ipv4();
int open_ping_socket_ipv6();
void socket_set_src_addr_ipv4(int s, FPING_INADDR src_addr);
void socket_set_src_addr_ipv6(int s, FPING_INADDR src_addr);
int socket_sendto_ping_ipv4(int s, struct sockaddr *saddr, socklen_t saddr_len, uint16_t icmp_seq_nr, uint16_t icmp_id_nr);
int socket_sendto_ping_ipv6(int s, struct sockaddr *saddr, socklen_t saddr_len, uint16_t icmp_seq_nr, uint16_t icmp_id_nr);

int open_ping_socket(size_t ping_data_size)
{
#ifndef IPV6
    return open_ping_socket_ipv4(ping_data_size);
#else
    return open_ping_socket_ipv6(ping_data_size);
#endif
}

void socket_set_src_addr(int s, FPING_INADDR src_addr)
{
#ifndef IPV6
    socket_set_src_addr_ipv4(s, src_addr);
#else
    socket_set_src_addr_ipv6(s, src_addr);
#endif
}

int socket_sendto_ping(int s, struct sockaddr *saddr, socklen_t saddr_len, uint16_t icmp_seq_nr, uint16_t icmp_id_nr)
{
#ifndef IPV6
    return socket_sendto_ping_ipv4(s, saddr, saddr_len, icmp_seq_nr, icmp_id_nr);
#else
    return socket_sendto_ping_ipv6(s, saddr, saddr_len, icmp_seq_nr, icmp_id_nr);
#endif
}
