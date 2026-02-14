#include "config.h"
#include "output.h"
#include "fping.h"
#include "flags.h"

#include <stdio.h>
#include <time.h>
#include <inttypes.h>

/************************************************************

  Function: ms_since_midnight_utc

*************************************************************

  Input: int64_t: current UTC time in ns

  Output: uint32_t: current time in ms since midnight UTC

  Description:

  Return ICMP Timestamp value corresponding to the given time value.
  The given time value must be in UTC.

*************************************************************/
static uint32_t ms_since_midnight_utc(int64_t time_val)
{
    return (uint32_t)((time_val / 1000000) % (24 * 60 * 60 * 1000));
}

/************************************************************

  Function: sprint_tm

*************************************************************

  render nanosecond int64_t value into milliseconds string with three digits of
  precision.

************************************************************/

const char *sprint_tm(int64_t ns)
{
    static char buf[10];
    double t = (double)ns / 1e6;

    if (t < 0.0) {
        /* negative (unexpected) */
        snprintf(buf, sizeof(buf), "%.2g", t);
    }
    else if (t < 1.0) {
        /* <= 0.99 ms */
        snprintf(buf, sizeof(buf), "%.3f", t);
    }
    else if (t < 10.0) {
        /* 1.00 - 9.99 ms */
        snprintf(buf, sizeof(buf), "%.2f", t);
    }
    else if (t < 100.0) {
        /* 10.0 - 99.9 ms */
        snprintf(buf, sizeof(buf), "%.1f", t);
    }
    else if (t < 1000000.0) {
        /* 100 - 1'000'000 ms */
        snprintf(buf, sizeof(buf), "%.0f", t);
    }
    else {
        snprintf(buf, sizeof(buf), "%.3e", t);
    }

    return (buf);
}

/************************************************************

  Function: print_human_readable_time from current_time_ns

*************************************************************/
void print_timestamp_format(int64_t current_time_ns, int timestamp_format)
{
    char time_buffer[100];
    time_t current_time_s;
    struct tm *local_time;

    current_time_s = current_time_ns / 1000000000;
    local_time = localtime(&current_time_s);
    switch(timestamp_format) {
        case 1:
            // timestamp-format ctime
            strftime(time_buffer, sizeof(time_buffer), "%c", local_time);
            if (opt_print_json_on)
                printf("\"timestamp\": \"%s\", ", time_buffer);
            else
                printf("[%s] ", time_buffer);
            break;
        case 2:
            // timestamp-format iso
            strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%dT%T%z", local_time);
            if (opt_print_json_on)
                printf("\"timestamp\": \"%s\", ", time_buffer);
            else
                printf("[%s] ", time_buffer);
            break;
        case 3:
            // timestamp-format rfc3339
            strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", local_time);
            if (opt_print_json_on)
                printf("\"timestamp\": \"%s\", ", time_buffer);
            else
                printf("[%s] ", time_buffer);
            break;
        default:
            if (opt_print_json_on)
                printf("\"timestamp\": \"%.5f\", ", (double)current_time_ns / 1e9);
            else
                printf("[%.5f] ", (double)current_time_ns / 1e9);
    }
}

/************************************************************

  Function: print_recv

*************************************************************

  Inputs: HOST_ENTRY *h, int64_t recv_time, int result,
          int this_count, int64_t this_reply, int avg

  Description:

************************************************************/

void print_recv(HOST_ENTRY *h, int64_t recv_time, int result, int this_count, int64_t this_reply, int avg) {
    if (opt_print_json_on) {
        printf("{\"resp\": {");

        if (opt_timestamp_on)
            print_timestamp_format(recv_time, opt_timestamp_format);

        printf("\"host\": \"%s\", ", h->host);
        printf("\"seq\": %d, ", this_count);
        printf("\"size\": %d, ", result);
        printf("\"rtt\": %s", sprint_tm(this_reply));
        return;
    }

    /* Normal Output */
    if (opt_timestamp_on)
        print_timestamp_format(recv_time, opt_timestamp_format);

    printf("%-*s : [%d], %d bytes, %s ms",
        max_hostname_len, h->host, this_count, result, sprint_tm(this_reply));

    printf(" (%s avg, ", sprint_tm(avg));

    if (h->num_recv <= h->num_sent) {
        printf("%d%% loss)",
            ((h->num_sent - h->num_recv) * 100) / h->num_sent);
    }
    else {
        printf("%d%% return)",
            (h->num_recv_total * 100) / h->num_sent);
    }
}

/************************************************************

  Function: print_timeout

*************************************************************

  Inputs: HOST_ENTRY *h, int ping_index

  Description:

************************************************************/

void print_timeout(HOST_ENTRY *h, int ping_index) {
    if (opt_print_json_on) {
        printf("{\"timeout\": {");
        if (opt_timestamp_on)
            print_timestamp_format(current_time_ns, opt_timestamp_format);

        printf("\"host\": \"%s\", ", h->host);
        printf("\"seq\": %d", ping_index);
        printf("}}\n");
        return;
    }

    /* Normal Output */
    if (opt_timestamp_on)
        print_timestamp_format(current_time_ns, opt_timestamp_format);

    printf("%-*s : [%d], timed out",
        max_hostname_len, h->host, ping_index);

    if (h->num_recv > 0) {
        printf(" (%s avg, ", sprint_tm(h->total_time / h->num_recv));
    }
    else {
        printf(" (NaN avg, ");
    }

    if (h->num_recv <= h->num_sent) {
        printf("%d%% loss)",
            ((h->num_sent - h->num_recv) * 100) / h->num_sent);
    }
    else {
        printf("%d%% return)",
            (h->num_recv_total * 100) / h->num_sent);
    }
    printf("\n");
}

/************************************************************

  Function: print_recv_ext

*************************************************************

  Inputs:  IP_HEADER_RESULT *ip_header_res,
           int64_t recv_time, int64_t this_reply

  Description:

************************************************************/

void print_recv_ext(IP_HEADER_RESULT *ip_header_res, int64_t recv_time, int64_t this_reply) { 
    if (opt_icmp_request_typ == 13) {
        printf("%s timestamps: Originate=%u Receive=%u Transmit=%u Localreceive=%u",
            opt_alive_on ? "" : ",",
            ip_header_res->otime_ms, ip_header_res->rtime_ms, ip_header_res->ttime_ms,
            ms_since_midnight_utc(recv_time));
    }

    if (ip_header_res->src_addr[0]) {
        printf(" (SRC %s)", ip_header_res->src_addr);
    }

#if defined(HAVE_IP_RECVTOS)
    if(opt_print_tos_on) {
        if(ip_header_res->tos != -1) {
            printf(" (TOS %d)", ip_header_res->tos);
        }
        else {
            printf(" (TOS unknown)");
        }
    }
#endif

    if (opt_print_ttl_on) {
        if(ip_header_res->ttl != -1) {
            printf(" (TTL %d)", ip_header_res->ttl);
        }
        else {
            printf(" (TTL unknown)");
        }
    }

    if (opt_elapsed_on && !opt_per_recv_on)
        printf(" (%s ms)", sprint_tm(this_reply));
    
    printf("\n");
}

/************************************************************

  Function: print_recv_ext_json

*************************************************************

  Inputs:  IP_HEADER_RESULT *ip_header_res,
           int64_t recv_time, int64_t this_reply

  Description:

************************************************************/

void print_recv_ext_json(IP_HEADER_RESULT *ip_header_res, int64_t recv_time, int64_t this_reply) {
    if (opt_icmp_request_typ == 13) {
        printf(", \"timestamps\": {");
        printf("\"originate\": %u, ", ip_header_res->otime_ms);
        printf("\"receive\": %u, ", ip_header_res->rtime_ms);
        printf("\"transmit\": %u, ", ip_header_res->ttime_ms);
        printf("\"localreceive\": %u}", ms_since_midnight_utc(recv_time));
    }

    if (ip_header_res->src_addr[0]) {
        printf(", \"src\": \"%s\"", ip_header_res->src_addr);
    }

#if defined(HAVE_IP_RECVTOS)
    if(opt_print_tos_on) {
        if(ip_header_res->tos != -1) {
            printf(", \"tos\": %d", ip_header_res->tos);
        }
        else {
            printf(", \"tos\": -1");
        }
    }
#endif

    if (opt_print_ttl_on) {
        if(ip_header_res->ttl != -1) {
            printf(", \"ttl\": %d", ip_header_res->ttl);
        }
        else {
            printf(", \"ttl\": -1");
        }
    }

    if (opt_elapsed_on && !opt_per_recv_on)
        printf(" (%s ms)", sprint_tm(this_reply));

    printf("}}");
    printf("\n");
}

/************************************************************

  Function: print_netdata

*************************************************************

  Inputs:  void (none)

  Description:


************************************************************/

void print_netdata(void)
{
    static int sent_charts = 0;

    int i;
    int64_t avg;
    HOST_ENTRY *h;

    for (i = 0; i < num_hosts; i++) {
        h = table[i];

        if (!sent_charts) {
            printf("CHART fping.%s_packets '' 'FPing Packets' packets '%s' fping.packets line 110020 %.0f\n", h->name, h->host, report_interval / 1e9);
            printf("DIMENSION xmt sent absolute 1 1\n");
            printf("DIMENSION rcv received absolute 1 1\n");
        }

        printf("BEGIN fping.%s_packets\n", h->name);
        printf("SET xmt = %d\n", h->num_sent_i);
        printf("SET rcv = %d\n", h->num_recv_i);
        printf("END\n");

        if (!sent_charts) {
            printf("CHART fping.%s_quality '' 'FPing Quality' percentage '%s' fping.quality area 110010 %.0f\n", h->name, h->host, report_interval / 1e9);
            printf("DIMENSION returned '' absolute 1 1\n");
        }

        printf("BEGIN fping.%s_quality\n", h->name);
        printf("SET returned = %d\n", h->num_sent_i > 0 ? ((h->num_recv_i * 100) / h->num_sent_i) : 0);
        printf("END\n");

        if (!sent_charts) {
            printf("CHART fping.%s_latency '' 'FPing Latency' ms '%s' fping.latency area 110000 %.0f\n", h->name, h->host, report_interval / 1e9);
            printf("DIMENSION min minimum absolute 1 1000000\n");
            printf("DIMENSION max maximum absolute 1 1000000\n");
            printf("DIMENSION avg average absolute 1 1000000\n");
        }

        printf("BEGIN fping.%s_latency\n", h->name);
        if (h->num_recv_i) {
            avg = h->total_time_i / h->num_recv_i;
            printf("SET min = %" PRId64 "\n", h->min_reply_i);
            printf("SET avg = %" PRId64 "\n", avg);
            printf("SET max = %" PRId64 "\n", h->max_reply_i);
        }
        printf("END\n");

        stats_reset_interval(h);
    }

    sent_charts = 1;
}

/************************************************************

  Function: print_per_system_splits

*************************************************************

  Inputs:  void (none)

  Description:


************************************************************/

void print_per_system_splits(void)
{
    int i, avg, outage_ms_i;
    HOST_ENTRY *h;
    struct tm *curr_tm;

    if (opt_verbose_on || opt_per_recv_on)
        fprintf(stderr, "\n");

    update_current_time();
    curr_tm = localtime((time_t *)&current_time.tv_sec);
    fprintf(stderr, "[%2.2d:%2.2d:%2.2d]\n", curr_tm->tm_hour,
        curr_tm->tm_min, curr_tm->tm_sec);

    for (i = 0; i < num_hosts; i++) {
        h = table[i];
        fprintf(stderr, "%-*s :", max_hostname_len, h->host);

        if (h->num_recv_i <= h->num_sent_i) {
            fprintf(stderr, " xmt/rcv/%%loss = %d/%d/%d%%",
                h->num_sent_i, h->num_recv_i, h->num_sent_i > 0 ? ((h->num_sent_i - h->num_recv_i) * 100) / h->num_sent_i : 0);

            if (opt_outage_on) {
                /* Time outage  */
                outage_ms_i = (h->num_sent_i - h->num_recv_i) * opt_perhost_interval / 1e6;
                fprintf(stderr, ", outage(ms) = %d", outage_ms_i);
            }
        }
        else {
            fprintf(stderr, " xmt/rcv/%%return = %d/%d/%d%%",
                h->num_sent_i, h->num_recv_i, h->num_sent_i > 0 ? ((h->num_recv_i * 100) / h->num_sent_i) : 0);
        }

        if (h->num_recv_i) {
            avg = h->total_time_i / h->num_recv_i;
            fprintf(stderr, ", min/avg/max = %s", sprint_tm(h->min_reply_i));
            fprintf(stderr, "/%s", sprint_tm(avg));
            fprintf(stderr, "/%s", sprint_tm(h->max_reply_i));
        }

        fprintf(stderr, "\n");
        if (!opt_cumulative_stats_on) {
            stats_reset_interval(h);
        }
    }
}

/************************************************************

  Function: print_per_system_splits_json

*************************************************************

  Inputs:  void (none)

  Description:


************************************************************/

void print_per_system_splits_json(void)
{
    int i, avg, outage_ms_i;
    HOST_ENTRY *h;

    update_current_time();

    for (i = 0; i < num_hosts; i++) {
        h = table[i];
        fprintf(stdout, "{\"intSum\": {");
        fprintf(stdout, "\"time\": %" PRId64 ",", current_time.tv_sec);
        fprintf(stdout, "\"host\": \"%s\", ", h->host);

        if (h->num_recv_i <= h->num_sent_i) {
            fprintf(stdout, "\"xmt\": %d, ", h->num_sent_i);
            fprintf(stdout, "\"rcv\": %d, ", h->num_recv_i);
            fprintf(stdout, "\"loss\": %d", h->num_sent_i > 0 ? ((h->num_sent_i - h->num_recv_i) * 100) / h->num_sent_i : 0);

            if (opt_outage_on) {
                /* Time outage  */
                outage_ms_i = (h->num_sent_i - h->num_recv_i) * opt_perhost_interval / 1e6;
                fprintf(stdout, ", \"outage(ms)\": %d", outage_ms_i);
            }
        }
        else {
            fprintf(stdout, "\"xmt\": %d, ", h->num_sent_i);
            fprintf(stdout, "\"rcv\": %d, ", h->num_recv_i);
            fprintf(stdout, "\"loss\": %d", h->num_sent_i > 0 ? ((h->num_recv_i * 100) / h->num_sent_i) : 0);
        }

        if (h->num_recv_i) {
            avg = h->total_time_i / h->num_recv_i;
            fprintf(stdout, ", \"rttMin\": %s, ", sprint_tm(h->min_reply_i));
            fprintf(stdout, "\"rttAvg\": %s, ", sprint_tm(avg));
            fprintf(stdout, "\"rttMax\": %s", sprint_tm(h->max_reply_i));
        }

        fprintf(stdout, "}}\n");
        if (!opt_cumulative_stats_on) {
            stats_reset_interval(h);
        }
    }
}
