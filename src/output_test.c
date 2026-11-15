#include "config.h"
#include "output.h"
#include "greatest.h"

#include <stdio.h>

static int capture_json_string(const char *input, char *buf, size_t buf_size)
{
    FILE *fp;
    size_t n;

    if (!buf || buf_size == 0) {
        return -1;
    }

    fp = tmpfile();
    if (!fp) {
        return -1;
    }

    print_json_string(fp, input);
    if (ferror(fp) || fflush(fp) != 0 || fseek(fp, 0, SEEK_SET) != 0) {
        fclose(fp);
        return -1;
    }

    n = fread(buf, 1, buf_size - 1, fp);
    /* Fail if output exceeded buf_size - 1 (fgetc detects unread trailing bytes). */
    if (ferror(fp) || (n == buf_size - 1 && (fgetc(fp) != EOF || ferror(fp)))) {
        fclose(fp);
        return -1;
    }

    buf[n] = '\0';
    fclose(fp);
    return 0;
}

TEST sprint_tm_returns_correct_value(void) {
    ASSERT_STR_EQ("-0.23", sprint_tm(-230900));
    ASSERT_STR_EQ("0.231", sprint_tm(230900));
    ASSERT_STR_EQ("2.31", sprint_tm(2309000));
    ASSERT_STR_EQ("23.1", sprint_tm(23090000));
    ASSERT_STR_EQ("231", sprint_tm(230900000));
    ASSERT_STR_EQ("2309", sprint_tm(2309000000));
    PASS();
}

TEST print_json_string_escapes_correctly(void)
{
    char buf[256];
    char high_byte[2];
    char expected_high[4];
    unsigned int c;

    ASSERT_EQ(0, capture_json_string(NULL, buf, sizeof(buf)));
    ASSERT_STR_EQ("\"\"", buf);

    ASSERT_EQ(0, capture_json_string("", buf, sizeof(buf)));
    ASSERT_STR_EQ("\"\"", buf);

    ASSERT_EQ(0, capture_json_string("127.0.0.1", buf, sizeof(buf)));
    ASSERT_STR_EQ("\"127.0.0.1\"", buf);

    /* Quotes and backslashes are escaped; solidus '/' (0x2F) passes through */
    ASSERT_EQ(0, capture_json_string("a\"\\b/c", buf, sizeof(buf)));
    ASSERT_STR_EQ("\"a\\\"\\\\b/c\"", buf);

    /* Printable ASCII boundaries: 0x20 (space) and 0x7F (DEL) pass through */
    ASSERT_EQ(0, capture_json_string(" \x7f", buf, sizeof(buf)));
    ASSERT_STR_EQ("\" \x7f\"", buf);

    /* Every control byte 0x01..0x1F uses its RFC 8259 short escape or lowercase \u00xx */
    for (c = 0x01; c <= 0x1f; c++) {
        char in[2];
        char expected[16];
        /* Must be static: greatest's ASSERT_*m() stores the raw pointer in
         * greatest_info.msg and returns before greatest_do_fail() prints it. */
        static char msg[64];

        in[0] = (char)c;
        in[1] = '\0';

        switch (c) {
        case '\b':
            snprintf(expected, sizeof(expected), "\"\\b\"");
            break;
        case '\f':
            snprintf(expected, sizeof(expected), "\"\\f\"");
            break;
        case '\n':
            snprintf(expected, sizeof(expected), "\"\\n\"");
            break;
        case '\r':
            snprintf(expected, sizeof(expected), "\"\\r\"");
            break;
        case '\t':
            snprintf(expected, sizeof(expected), "\"\\t\"");
            break;
        default:
            snprintf(expected, sizeof(expected), "\"\\u%04x\"", c);
            break;
        }

        snprintf(msg, sizeof(msg), "control byte 0x%02x", c);
        ASSERT_EQm(msg, 0, capture_json_string(in, buf, sizeof(buf)));
        ASSERT_STR_EQm(msg, expected, buf);
    }

    /* Valid multi-byte UTF-8 passes through while ASCII quotes are escaped */
    ASSERT_EQ(0, capture_json_string("ok\xc3\xa4\"end\xe2\x82\xac\xf0\x9f\x98\x80", buf, sizeof(buf)));
    ASSERT_STR_EQ("\"ok\xc3\xa4\\\"end\xe2\x82\xac\xf0\x9f\x98\x80\"", buf);

    /* 0xFF is (char)-1 (EOF on signed-char platforms) and must pass through unescaped */
    high_byte[0] = (char)0xff;
    high_byte[1] = '\0';
    expected_high[0] = '"';
    expected_high[1] = (char)0xff;
    expected_high[2] = '"';
    expected_high[3] = '\0';
    ASSERT_EQ(0, capture_json_string(high_byte, buf, sizeof(buf)));
    ASSERT_STR_EQ(expected_high, buf);

    PASS();
}

SUITE(suite) {
    RUN_TEST(sprint_tm_returns_correct_value);
    RUN_TEST(print_json_string_escapes_correctly);
}

/* Add definitions that need to be in the test runner's main file. */
GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();      /* init & parse command-line args */
    RUN_SUITE(suite);
    GREATEST_MAIN_END();        /* display results */
}

