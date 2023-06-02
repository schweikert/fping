
#ifndef FPING_DATA_TYPES_H
#define FPING_DATA_TYPES_H

#include <inttypes.h>
#include <sys/socket.h>

struct event;
typedef struct host_entry {
    int i; /* index into array */
    char *name; /* name as given by user */
    char *host; /* text description of host */
    struct sockaddr_storage saddr; /* internet address */
    socklen_t saddr_len;
    int64_t timeout; /* time to wait for response */
    int64_t last_send_time; /* time of last packet sent */
    int num_sent; /* number of ping packets sent (for statistics) */
    int num_recv; /* number of pings received (duplicates ignored) */
    int num_recv_total; /* number of pings received, including duplicates */
    int64_t max_reply; /* longest response time */
    int64_t min_reply; /* shortest response time */
    int64_t total_time; /* sum of response times */
    /* _i -> splits (reset on every report interval) */
    int num_sent_i; /* number of ping packets sent */
    int num_recv_i; /* number of pings received */
    int64_t max_reply_i; /* longest response time */
    int64_t min_reply_i; /* shortest response time */
    int64_t total_time_i; /* sum of response times */
    int64_t *resp_times; /* individual response times */

    

    int     top_view_print_pos;               /* line where the host is printed */
    int64_t top_view_last_timeouts;           /* timeout Counter */
    int64_t top_view_last_timeouts_count;     /* how many timeout occurred */
    int64_t top_view_last_timeouts_seq;       /* how many packets where lost till the next answer arrived */
    char    top_view_last_timeout_time[100];  /* buffer to print how long the last timeout was ( packets * period ) */

    char    top_view_icmp_message[200];  /* buffer to print how long the last timeout was ( packets * period ) */

    /* to avoid allocating two struct events each time that we send a ping, we
     * preallocate here two struct events for each ping that we might send for
     * this host. */
    struct event *event_storage_ping;
    struct event *event_storage_timeout;
} HOST_ENTRY;


#endif
