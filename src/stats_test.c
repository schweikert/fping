#include "config.h"

#include <stdint.h>

#include "greatest.h"
#include "stats.h"

TEST calc_pct_cases(void)
{
    ASSERT_EQ(0, calc_pct(0, 0));
    ASSERT_EQ(0, calc_pct(5, 0));
    ASSERT_EQ(0, calc_pct(5, -10));
    ASSERT_EQ(0, calc_pct(-1, 10));
    ASSERT_EQ(0, calc_pct(0, 10));
    ASSERT_EQ(100, calc_pct(10, 10));
    ASSERT_EQ(100, calc_pct(15, 10));

    ASSERT_EQ(50, calc_pct(1, 2));
    ASSERT_EQ(33, calc_pct(1, 3));
    ASSERT_EQ(66, calc_pct(2, 3));

    /* Above 32-bit int * 100 boundary (INT_MAX / 100 = 21,474,836) */
    ASSERT_EQ(50, calc_pct(50000000LL, 100000000LL));
    ASSERT_EQ(99, calc_pct(2999999999LL, 3000000000LL));
    PASS();
}

TEST calc_loss_pct_cases(void)
{
    ASSERT_EQ(0, calc_loss_pct(0, 0));
    ASSERT_EQ(0, calc_loss_pct(-5, 0));
    ASSERT_EQ(0, calc_loss_pct(10, 10));
    ASSERT_EQ(0, calc_loss_pct(10, 12));
    ASSERT_EQ(100, calc_loss_pct(10, 0));
    ASSERT_EQ(100, calc_loss_pct(10, -1));
    ASSERT_EQ(25, calc_loss_pct(4, 3));

    /* Exceeds 32-bit (sent - recv) * 100 */
    ASSERT_EQ(50, calc_loss_pct(100000000LL, 50000000LL));
    PASS();
}

TEST calc_outage_ms_cases(void)
{
    /* Guards */
    ASSERT_EQ(0, calc_outage_ms(0, 0, 1000000000LL));
    ASSERT_EQ(0, calc_outage_ms(5, 5, 1000000000LL));
    ASSERT_EQ(0, calc_outage_ms(5, 6, 1000000000LL));
    ASSERT_EQ(0, calc_outage_ms(5, -1, 1000000000LL));
    ASSERT_EQ(0, calc_outage_ms(5, 0, 0));
    ASSERT_EQ(0, calc_outage_ms(5, 0, -1000000LL));

    /* Standard 1000 ms interval */
    ASSERT_EQ(3000, calc_outage_ms(5, 2, 1000000000LL));

    /* Fractional-ms interval (-p 10.5 -> 10,500,000 ns) truncates toward zero */
    ASSERT_EQ(10, calc_outage_ms(1, 0, 10500000LL));
    ASSERT_EQ(21, calc_outage_ms(2, 0, 10500000LL));
    ASSERT_EQ(31, calc_outage_ms(3, 0, 10500000LL));

    /* Sub-ms interval (-p 0.1 -> 100,000 ns) */
    ASSERT_EQ(100000, calc_outage_ms(1000000LL, 0, 100000LL));

    /* > 24.85 days (exceeds 32-bit INT_MAX ms = 2,147,483,647): 30 days at 1s interval */
    int64_t thirty_days_s = 30LL * 24 * 3600;
    ASSERT_EQ(thirty_days_s * 1000LL, calc_outage_ms(thirty_days_s, 0, 1000000000LL));
    PASS();
}

SUITE(stats_suite)
{
    RUN_TEST(calc_pct_cases);
    RUN_TEST(calc_loss_pct_cases);
    RUN_TEST(calc_outage_ms_cases);
}

GREATEST_MAIN_DEFS();

int main(int argc, char **argv)
{
    GREATEST_MAIN_BEGIN();
    RUN_SUITE(stats_suite);
    GREATEST_MAIN_END();
}
