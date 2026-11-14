#include "config.h"
#include "stats.h"
#include "fping.h"
#include "flags.h"
#include "output.h"

#include <stdio.h>
#include <inttypes.h>

/* Return (part * 100) / total in [0, 100], truncated toward zero (0 if total <= 0). */
int calc_pct(int64_t part, int64_t total)
{
    if (total <= 0 || part <= 0)
        return 0;
    if (part >= total)
        return 100;
    return (int)((part * 100) / total);
}

/* Return packet loss percentage in [0, 100], truncated toward zero. */
int calc_loss_pct(int64_t sent, int64_t recv)
{
    if (sent <= 0 || recv >= sent)
        return 0;
    if (recv <= 0)
        return 100;
    return calc_pct(sent - recv, sent);
}

/* Return estimated outage duration in ms for (sent - recv) lost packets,
 * truncated toward zero. */
int64_t calc_outage_ms(int64_t sent, int64_t recv, int64_t perhost_interval_ns)
{
    if (sent <= recv || recv < 0 || perhost_interval_ns <= 0)
        return 0;
    int64_t lost = sent - recv;
    return lost * (perhost_interval_ns / 1000000)
        + (lost * (perhost_interval_ns % 1000000)) / 1000000;
}

/************************************************************

  Function: print_per_system_stats

*************************************************************

  Inputs:  void (none)

  Description:


************************************************************/

void print_per_system_stats(void)
{
    int i, j;
    int64_t avg, outage_ms;
    HOST_ENTRY *h;
    int64_t resp;

    if (opt.verbose_on || opt.per_recv_on)
        fprintf(stderr, "\n");

    for (i = 0; i < num_hosts; i++) {
        h = table[i];
        fprintf(stderr, "%-*s :", max_hostname_len, h->host);

        if (opt.report_all_rtts_on) {
            for (j = 0; j < h->num_sent; j++) {
                if ((resp = h->resp_times[j]) >= 0)
                    fprintf(stderr, " %s", sprint_tm(resp));
                else
                    fprintf(stderr, " -");
            }

            fprintf(stderr, "\n");
        }
        else {
            fprintf(stderr, " xmt/rcv/%%loss = %d/%d/%d%%",
                h->num_sent, h->num_recv, calc_loss_pct(h->num_sent, h->num_recv));

            if (opt.outage_on) {
                /* Time outage total */
                outage_ms = calc_outage_ms(h->num_sent, h->num_recv, opt.perhost_interval);
                fprintf(stderr, ", outage(ms) = %" PRId64, outage_ms);
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

  Function: print_per_system_stats_json

*************************************************************

  Inputs:  void (none)

  Description:


************************************************************/

void print_per_system_stats_json(void)
{
    int i, j;
    int64_t avg, outage_ms;
    HOST_ENTRY *h;
    int64_t resp;

    for (i = 0; i < num_hosts; i++) {
        h = table[i];

        if (opt.report_all_rtts_on)
            fprintf(stdout, "{\"vSum\": {");
        else
            fprintf(stdout, "{\"summary\": {");

        fprintf(stdout, "\"host\": ");
        print_json_string(stdout, h->host);
        fprintf(stdout, ", ");

        if (opt.report_all_rtts_on) {
            fprintf(stdout, "\"values\": [");
            for (j = 0; j < h->num_sent; j++) {
                if (j > 0)
                  fprintf(stdout, ", ");
                
                if ((resp = h->resp_times[j]) >= 0)
                    fprintf(stdout, "%s", sprint_tm(resp));
                else
                    fprintf(stdout, "null");
            }

            fprintf(stdout, "]}");
        }
        else {
            fprintf(stdout, "\"xmt\": %d, ", h->num_sent);
            fprintf(stdout, "\"rcv\": %d, ", h->num_recv);
            fprintf(stdout, "\"loss\": %d", calc_loss_pct(h->num_sent, h->num_recv));

            if (opt.outage_on) {
                /* Time outage total */
                outage_ms = calc_outage_ms(h->num_sent, h->num_recv, opt.perhost_interval);
                fprintf(stdout, ", \"outage(ms)\": %" PRId64, outage_ms);
            }

            if (h->num_recv) {
                avg = h->total_time / h->num_recv;
                fprintf(stdout, ", \"rttMin\": %s", sprint_tm(h->min_reply));
                fprintf(stdout, ", \"rttAvg\": %s", sprint_tm(avg));
                fprintf(stdout, ", \"rttMax\": %s", sprint_tm(h->max_reply));
            }

            fprintf(stdout, "}");
        }
        fprintf(stdout, "}\n");
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

/************************************************************

  Function: print_global_stats_json

*************************************************************

  Inputs:  void (none)

  Description:


************************************************************/

void print_global_stats_json(void)
{
    fprintf(stdout, "{\"stats\": {");
    fprintf(stdout, "\"targets\": %d, ", num_hosts);
    fprintf(stdout, "\"alive\": %d, ", num_alive);
    fprintf(stdout, "\"unreachable\": %d, ", num_unreachable);
    fprintf(stdout, "\"unknownAddresses\": %d, ", num_noaddress);
    fprintf(stdout, "\"timeouts\": %d, ", num_timeout);
    fprintf(stdout, "\"icmpEchosSent\": %d, ", num_pingsent);
    fprintf(stdout, "\"icmpEchoRepliesReceived\": %d, ", num_pingreceived);
    fprintf(stdout, "\"otherIcmpReceived\": %d, ", num_othericmprcvd);

    if (total_replies == 0) {
        min_reply = 0;
        max_reply = 0;
        total_replies = 1;
        sum_replies = 0;
    }

    fprintf(stdout, "\"rttMin\": %s, ", sprint_tm(min_reply));
    fprintf(stdout, "\"rttAvg\": %s, ", sprint_tm(sum_replies / total_replies));
    fprintf(stdout, "\"rttMax\": %s, ", sprint_tm(max_reply));
    fprintf(stdout, "\"elapsed\": %.3f", (end_time - start_time) / 1e9);
    fprintf(stdout, "}}\n");
}
