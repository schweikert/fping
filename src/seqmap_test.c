#include "config.h"

#include "greatest.h"
#include "seqmap.h"

TEST seqmap_expiry_and_wraparound(void)
{
    /* Fixed timestamps keep the test deterministic and avoid sleeps. */
    const int64_t timeout = 10;
    const int64_t start = 100;
    SEQMAP_VALUE *value;

    seqmap_init(timeout);

    /* The first sequence number should be 0. */
    ASSERT_EQ(0, seqmap_add(7, 10, start));

    /* Fill the remaining sequence-number slots. */
    for (unsigned int i = 1; i < SEQMAP_MAXSEQ; i++) {
        ASSERT_EQ(i, seqmap_add(i, i, start + 1));
    }

    /* Entry 0 is valid just before its timeout. */
    value = seqmap_fetch(0, start + timeout - 1);
    ASSERT(value != NULL);
    ASSERT_EQ(7, value->host_nr);

    /* Entry 0 has expired at the timeout boundary. */
    ASSERT(seqmap_fetch(0, start + timeout) == NULL);

    /* Adding another entry wraps to ID 0 and reuses its expired slot. */
    ASSERT_EQ(0, seqmap_add(42, 99, start + timeout));

    /* ID 0 now refers to the new entry. */
    value = seqmap_fetch(0, start + timeout);
    ASSERT(value != NULL);
    ASSERT_EQ(42, value->host_nr);
    ASSERT_EQ(99, value->ping_count);

    PASS();
}

SUITE(seqmap_suite)
{
    RUN_TEST(seqmap_expiry_and_wraparound);
}

GREATEST_MAIN_DEFS();

int main(int argc, char **argv)
{
    GREATEST_MAIN_BEGIN();
    RUN_SUITE(seqmap_suite);
    GREATEST_MAIN_END();
}