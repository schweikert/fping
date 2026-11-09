#include "config.h"

#include "fping.h"
#include "greatest.h"
#include "options.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <stdlib.h>
#include <string.h>

TEST calcsum_odd_length_no_overread_or_sign_extension(void)
{
    /* Exact-sized malloc() buffers let ASan catch any 1-byte over-read,
     * and trailing bytes >= 0x80 catch signed-char sign extension. */
    {
        unsigned char *buf = malloc(1);
        ASSERT(buf != NULL);
        buf[0] = 0x80;
        ASSERT_EQ(htons(0x7FFF), calcsum((unsigned short *)buf, 1));
        free(buf);
    }

    {
        unsigned char *buf = malloc(1);
        ASSERT(buf != NULL);
        buf[0] = 0xFF;
        ASSERT_EQ(htons(0x00FF), calcsum((unsigned short *)buf, 1));
        free(buf);
    }

    {
        unsigned char *buf = malloc(3);
        ASSERT(buf != NULL);
        buf[0] = 0x00;
        buf[1] = 0x01;
        buf[2] = 0x80;
        ASSERT_EQ(htons(0x7FFE), calcsum((unsigned short *)buf, 3));
        free(buf);
    }

    PASS();
}

TEST calcsum_roundtrip_verifies_to_zero(void)
{
    /* Odd-length (9-byte) ICMP Echo packet with trailing byte >= 0x80 */
    size_t len = ICMP_MINLEN + 1;
    unsigned char *pkt = calloc(1, len);
    ASSERT(pkt != NULL);

    struct icmp hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.icmp_type = ICMP_ECHO;
    hdr.icmp_id = htons(0x1234);
    hdr.icmp_seq = htons(1);
    memcpy(pkt, &hdr, ICMP_MINLEN);
    pkt[ICMP_MINLEN] = 0xAB;

    uint16_t cksum = calcsum((unsigned short *)pkt, (int)len);
    hdr.icmp_cksum = cksum;
    memcpy(pkt, &hdr, ICMP_MINLEN);

    ASSERT_EQ(0, calcsum((unsigned short *)pkt, (int)len));
    free(pkt);
    PASS();
}

TEST sendto_icmp_tstamp_with_random_preserves_timestamps(void)
{
    memset(&opt, 0, sizeof(opt));
    opt.random_data_on = 1;

    init_ping_buffer_ipv4(ICMP_TIMESTAMP_DATA_SIZE);

    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &sa.sin_addr);

    /* Pass invalid fd -1 so sendto() returns -1 (EBADF) right after
     * constructing the ICMP Timestamp packet in ping_buffer_ipv4. */
    (void)socket_sendto_ping_ipv4(-1, (struct sockaddr *)&sa, sizeof(sa), 7, htons(0x4242), ICMP_TSTAMP);

    struct icmp icp;
    ASSERT_EQ((size_t)(ICMP_MINLEN + ICMP_TIMESTAMP_DATA_SIZE), ping_pkt_size_ipv4);
    memcpy(&icp, ping_buffer_ipv4, ICMP_MINLEN + ICMP_TIMESTAMP_DATA_SIZE);

    ASSERT_EQ(ICMP_TSTAMP, icp.icmp_type);
    ASSERT_EQ(0, icp.icmp_code);
    ASSERT_EQ(htons(7), icp.icmp_seq);
    ASSERT_EQ(htons(0x4242), icp.icmp_id);
    /* Originate timestamp must be valid ms since midnight UTC (< 86400000),
     * and Receive/Transmit timestamps must remain 0 (not overwritten by -R). */
    ASSERT(ntohl(icp.icmp_otime) < 86400000U);
    ASSERT_EQ(0U, ntohl(icp.icmp_rtime));
    ASSERT_EQ(0U, ntohl(icp.icmp_ttime));
    ASSERT_EQ(0, calcsum((unsigned short *)ping_buffer_ipv4, (int)ping_pkt_size_ipv4));

    free(ping_buffer_ipv4);
    ping_buffer_ipv4 = NULL;
    PASS();
}

TEST sendto_icmp_echo_odd_random_valid_checksum(void)
{
    memset(&opt, 0, sizeof(opt));
    opt.random_data_on = 1;

    /* Odd payload size (5 bytes -> 13-byte ICMP packet) */
    init_ping_buffer_ipv4(5);

    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &sa.sin_addr);

    (void)socket_sendto_ping_ipv4(-1, (struct sockaddr *)&sa, sizeof(sa), 11, htons(0x1337), ICMP_ECHO);

    ASSERT_EQ((size_t)(ICMP_MINLEN + 5), ping_pkt_size_ipv4);
    ASSERT_EQ(0, calcsum((unsigned short *)ping_buffer_ipv4, (int)ping_pkt_size_ipv4));

    free(ping_buffer_ipv4);
    ping_buffer_ipv4 = NULL;
    PASS();
}

SUITE(socket4_suite)
{
    RUN_TEST(calcsum_odd_length_no_overread_or_sign_extension);
    RUN_TEST(calcsum_roundtrip_verifies_to_zero);
    RUN_TEST(sendto_icmp_tstamp_with_random_preserves_timestamps);
    RUN_TEST(sendto_icmp_echo_odd_random_valid_checksum);
}

GREATEST_MAIN_DEFS();

int main(int argc, char **argv)
{
    GREATEST_MAIN_BEGIN();
    RUN_SUITE(socket4_suite);
    GREATEST_MAIN_END();
}
