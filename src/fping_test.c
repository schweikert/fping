#include "config.h"

#include "fping.h"
#include "greatest.h"
#include "seqmap.h"

#include <stdint.h>
#include <string.h>

TEST calc_loop_event_storage_count_normal_cases(void)
{
    /* perhost_interval > timeout or interval > timeout */
    ASSERT_EQ(1, calc_loop_event_storage_count(500000000, 1000000000, 10000000));
    ASSERT_EQ(1, calc_loop_event_storage_count(500000000, 10000000, 1000000000));

    /* timeout == min_interval */
    ASSERT_EQ(2, calc_loop_event_storage_count(500000000, 500000000, 10000000));

    /* perhost_interval >= interval */
    ASSERT_EQ(6, calc_loop_event_storage_count(500000000, 100000000, 10000000));

    /* perhost_interval < interval: min_interval is bounded by interval */
    ASSERT_EQ(51, calc_loop_event_storage_count(500000000, 1000000, 10000000));

    /* perhost_interval == 0, interval > 0 */
    ASSERT_EQ(51, calc_loop_event_storage_count(500000000, 0, 10000000));

    PASS();
}

TEST calc_loop_event_storage_count_zero_and_negative(void)
{
    /* Non-positive timeout */
    ASSERT_EQ(1, calc_loop_event_storage_count(0, 10000000, 1000000));
    ASSERT_EQ(1, calc_loop_event_storage_count(-1, 10000000, 1000000));
    ASSERT_EQ(1, calc_loop_event_storage_count(INT64_MIN, 0, 0));

    /* Non-positive effective interval (-l -p 0 -i 0 must not divide by zero) */
    ASSERT_EQ(SEQMAP_MAXSEQ, calc_loop_event_storage_count(500000000, 0, 0));
    ASSERT_EQ(SEQMAP_MAXSEQ, calc_loop_event_storage_count(500000000, -1, -1));

    PASS();
}

TEST calc_loop_event_storage_count_seqmap_cap_and_overflow(void)
{
    /* Boundary values around SEQMAP_MAXSEQ */
    ASSERT_EQ(SEQMAP_MAXSEQ - 1, calc_loop_event_storage_count(SEQMAP_MAXSEQ - 2, 1, 0));
    ASSERT_EQ(SEQMAP_MAXSEQ, calc_loop_event_storage_count(SEQMAP_MAXSEQ - 1, 1, 0));
    ASSERT_EQ(SEQMAP_MAXSEQ, calc_loop_event_storage_count(SEQMAP_MAXSEQ, 1, 0));

    /* Large timeout with perhost_interval == 0, interval > 0 */
    ASSERT_EQ(SEQMAP_MAXSEQ, calc_loop_event_storage_count(1000000000000LL, 0, 1000000));

    /* Non-root SIGFPE reproducer (-l -p 10 -t 42949672950): 1 + timeout/p == 0x100000000LL */
    ASSERT_EQ(SEQMAP_MAXSEQ, calc_loop_event_storage_count(42949672950000000LL, 10000000LL, 1000000LL));

    /* Extreme overflow check */
    ASSERT_EQ(SEQMAP_MAXSEQ, calc_loop_event_storage_count(INT64_MAX, 1, 0));

    PASS();
}

TEST stats_add_preserves_zero_min_reply(void)
{
    HOST_ENTRY h;
    int64_t resp_times[4];

    memset(&opt, 0, sizeof(opt));
    memset(&h, 0, sizeof(h));
    h.resp_times = resp_times;

    /* First reply has 0 ns RTT: subsequent larger RTT must not overwrite min */
    stats_add(&h, 0, 1, 0);
    stats_add(&h, 1, 1, 5000);
    ASSERT_EQ(2, h.num_recv);
    ASSERT_EQ(0, h.min_reply);
    ASSERT_EQ(0, h.min_reply_i);
    ASSERT_EQ(5000, h.max_reply);
    ASSERT_EQ(5000, h.max_reply_i);

    /* Same behavior after resetting interval stats */
    stats_reset_interval(&h);
    stats_add(&h, 2, 1, 0);
    stats_add(&h, 3, 1, 8000);
    ASSERT_EQ(0, h.min_reply_i);
    ASSERT_EQ(8000, h.max_reply_i);

    /* Descending non-zero -> smaller -> zero updates min properly */
    memset(&h, 0, sizeof(h));
    h.resp_times = resp_times;
    stats_add(&h, 0, 1, 5000);
    ASSERT_EQ(5000, h.min_reply);
    stats_add(&h, 1, 1, 1000);
    ASSERT_EQ(1000, h.min_reply);
    stats_add(&h, 2, 1, 0);
    ASSERT_EQ(0, h.min_reply);

    PASS();
}

SUITE(fping_suite)
{
    RUN_TEST(calc_loop_event_storage_count_normal_cases);
    RUN_TEST(calc_loop_event_storage_count_zero_and_negative);
    RUN_TEST(calc_loop_event_storage_count_seqmap_cap_and_overflow);
    RUN_TEST(stats_add_preserves_zero_min_reply);
}

GREATEST_MAIN_DEFS();

int main(int argc, char **argv)
{
    GREATEST_MAIN_BEGIN();
    RUN_SUITE(fping_suite);
    GREATEST_MAIN_END();
}
