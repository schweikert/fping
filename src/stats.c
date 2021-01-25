#include "fping.h"


/************************************************************

  Function: print_per_system_stats

*************************************************************

  Inputs:  void (none)

  Description:


************************************************************/

void print_per_system_stats(void)
{
    int i, j, avg, outage_ms;
    HOST_ENTRY* h;
    int64_t resp;

    if (verbose_flag || per_recv_flag)
        fprintf(stderr, "\n");

    for (i = 0; i < num_hosts; i++) {
        h = table[i];
        fprintf(stderr, "%-*s :", max_hostname_len, h->host);

        if (report_all_rtts_flag) {
            for (j = 0; j < h->num_sent; j++) {
                if ((resp = h->resp_times[j]) >= 0)
                    fprintf(stderr, " %s", sprint_tm(resp));
                else
                    fprintf(stderr, " -");
            }

            fprintf(stderr, "\n");
        }
        else {
            if (h->num_recv <= h->num_sent) {
                fprintf(stderr, " xmt/rcv/%%loss = %d/%d/%d%%",
                    h->num_sent, h->num_recv, h->num_sent > 0 ? ((h->num_sent - h->num_recv) * 100) / h->num_sent : 0);

                if (outage_flag) {
                    /* Time outage total */
                    outage_ms = (h->num_sent - h->num_recv) * perhost_interval / 1e6;
                    fprintf(stderr, ", outage(ms) = %d", outage_ms);
                }
            }
            else {
                fprintf(stderr, " xmt/rcv/%%return = %d/%d/%d%%",
                    h->num_sent, h->num_recv,
                    ((h->num_recv * 100) / h->num_sent));
            }

            if (h->num_recv) {
                avg = h->total_time / h->num_recv;
                fprintf(stderr, ", min/avg/max = %s", sprint_tm(h->min_reply));
                fprintf(stderr, "/%s", sprint_tm(avg));
                fprintf(stderr, "/%s", sprint_tm(h->max_reply));
            }

            fprintf(stderr, "\n");
        }
    }
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
    HOST_ENTRY* h;

    for (i = 0; i < num_hosts; i++) {
        h = table[i];

        if (!sent_charts) {
            printf("CHART fping.%s_packets '' 'FPing Packets for host %s' packets '%s' fping.packets line 110020 %.0f\n", h->name, h->host, h->name, report_interval / 1e9);
            printf("DIMENSION xmt sent absolute 1 1\n");
            printf("DIMENSION rcv received absolute 1 1\n");
        }

        printf("BEGIN fping.%s_packets\n", h->name);
        printf("SET xmt = %d\n", h->num_sent_i);
        printf("SET rcv = %d\n", h->num_recv_i);
        printf("END\n");

        if (!sent_charts) {
            printf("CHART fping.%s_quality '' 'FPing Quality for host %s' percentage '%s' fping.quality area 110010 %.0f\n", h->name, h->host, h->name, report_interval / 1e9);
            printf("DIMENSION returned '' absolute 1 1\n");
            /* printf("DIMENSION lost '' absolute 1 1\n"); */
        }

        printf("BEGIN fping.%s_quality\n", h->name);
        /*
        if( h->num_recv_i <= h->num_sent_i )
            printf("SET lost = %d\n", h->num_sent_i > 0 ? ( ( h->num_sent_i - h->num_recv_i ) * 100 ) / h->num_sent_i : 0 );
        else
            printf("SET lost = 0\n");
*/

        printf("SET returned = %d\n", h->num_sent_i > 0 ? ((h->num_recv_i * 100) / h->num_sent_i) : 0);
        printf("END\n");

        if (!sent_charts) {
            printf("CHART fping.%s_latency '' 'FPing Latency for host %s' ms '%s' fping.latency area 110000 %.0f\n", h->name, h->host, h->name, report_interval / 1e9);
            printf("DIMENSION min minimum absolute 1 1000000\n");
            printf("DIMENSION max maximum absolute 1 1000000\n");
            printf("DIMENSION avg average absolute 1 1000000\n");
        }

        printf("BEGIN fping.%s_latency\n", h->name);
        if (h->num_recv_i) {
            avg = h->total_time_i / h->num_recv_i;
            printf("SET min = %ld\n", h->min_reply_i);
            printf("SET avg = %ld\n", avg);
            printf("SET max = %ld\n", h->max_reply_i);
        }
        printf("END\n");

        h->num_sent_i = h->num_recv_i = h->max_reply_i = h->min_reply_i = h->total_time_i = 0;
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
    HOST_ENTRY* h;
    struct tm* curr_tm;

    if (verbose_flag || per_recv_flag)
        fprintf(stderr, "\n");

    update_current_time();
    curr_tm = localtime((time_t*)&current_time.tv_sec);
    fprintf(stderr, "[%2.2d:%2.2d:%2.2d]\n", curr_tm->tm_hour,
        curr_tm->tm_min, curr_tm->tm_sec);

    for (i = 0; i < num_hosts; i++) {
        h = table[i];
        fprintf(stderr, "%-*s :", max_hostname_len, h->host);

        if (h->num_recv_i <= h->num_sent_i) {
            fprintf(stderr, " xmt/rcv/%%loss = %d/%d/%d%%",
                h->num_sent_i, h->num_recv_i, h->num_sent_i > 0 ? ((h->num_sent_i - h->num_recv_i) * 100) / h->num_sent_i : 0);

            if (outage_flag) {
                /* Time outage  */
                outage_ms_i = (h->num_sent_i - h->num_recv_i) * perhost_interval / 1e6;
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
        h->num_sent_i = h->num_recv_i = h->max_reply_i = h->min_reply_i = h->total_time_i = 0;
    }
}

/************************************************************

  Function: print_global_stats

*************************************************************

  Inputs:  void (none)

  Description:


************************************************************/

void print_global_stats(void)
{
    fprintf(stderr, "\n");
    fprintf(stderr, " %7d targets\n", num_hosts);
    fprintf(stderr, " %7d alive\n", num_alive);
    fprintf(stderr, " %7d unreachable\n", num_unreachable);
    fprintf(stderr, " %7d unknown addresses\n", num_noaddress);
    fprintf(stderr, "\n");
    fprintf(stderr, " %7d timeouts (waiting for response)\n", num_timeout);
    fprintf(stderr, " %7d ICMP Echos sent\n", num_pingsent);
    fprintf(stderr, " %7d ICMP Echo Replies received\n", num_pingreceived);
    fprintf(stderr, " %7d other ICMP received\n", num_othericmprcvd);
    fprintf(stderr, "\n");

    if (total_replies == 0) {
        min_reply = 0;
        max_reply = 0;
        total_replies = 1;
        sum_replies = 0;
    }

    fprintf(stderr, " %s ms (min round trip time)\n", sprint_tm(min_reply));
    fprintf(stderr, " %s ms (avg round trip time)\n",
        sprint_tm(sum_replies / total_replies));
    fprintf(stderr, " %s ms (max round trip time)\n", sprint_tm(max_reply));
    fprintf(stderr, " %12.3f sec (elapsed real time)\n",
        (end_time - start_time) / 1e9);
    fprintf(stderr, "\n");
}
