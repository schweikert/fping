#include "config.h"
#include "output.h"
#include "greatest.h"

TEST sprint_tm_returns_correct_value(void) {
    ASSERT_STR_EQ("-0.23", sprint_tm(-230900));
    ASSERT_STR_EQ("0.231", sprint_tm(230900));
    ASSERT_STR_EQ("2.31", sprint_tm(2309000));
    ASSERT_STR_EQ("23.1", sprint_tm(23090000));
    ASSERT_STR_EQ("231", sprint_tm(230900000));
    ASSERT_STR_EQ("2309", sprint_tm(2309000000));
    PASS();
}

SUITE(suite) {
    RUN_TEST(sprint_tm_returns_correct_value);
}

/* Add definitions that need to be in the test runner's main file. */
GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();      /* init & parse command-line args */
    RUN_SUITE(suite);
    GREATEST_MAIN_END();        /* display results */
}

