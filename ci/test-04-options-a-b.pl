#!/usr/bin/perl -w

use Test::Command tests => 77;
use Test::More;
use Time::HiRes qw(gettimeofday tv_interval);

#  -4         only use IPv4 addresses
#  -6         only use IPv6 addresses
#  -a         show targets that are alive
#  -A         show targets by address
#  -b n       amount of ping data to send, in bytes (default 56)
#  -B f       set exponential backoff factor to f

# fping -4 -6
{
my $cmd = Test::Command->new(cmd => "fping -4 -6 127.0.0.1");
$cmd->exit_is_num(1);
$cmd->stdout_is_eq("");
$cmd->stderr_is_eq("fping: can't specify both -4 and -6\n");
}

# fping -6 -4
{
my $cmd = Test::Command->new(cmd => "fping -6 -4 127.0.0.1");
$cmd->exit_is_num(1);
$cmd->stdout_is_eq("");
$cmd->stderr_is_eq("fping: can't specify both -4 and -6\n");
}

# fping -4
{
my $cmd = Test::Command->new(cmd => "fping -4 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("127.0.0.1 is alive\n");
$cmd->stderr_is_eq("");
}

{
my $cmd = Test::Command->new(cmd => "fping -4 ::1");
$cmd->exit_is_num(2);
$cmd->stdout_is_eq("");
$cmd->stderr_like(qr{^::1: .*(?i:name|address|host|resolv).*\n$});
}

# fping -6
SKIP: {
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 3;
    }
    my $cmd = Test::Command->new(cmd => "fping -6 ::1");
    $cmd->exit_is_num(0);
    $cmd->stdout_is_eq("::1 is alive\n");
    $cmd->stderr_is_eq("");
}

{
my $cmd = Test::Command->new(cmd => "fping -6 127.0.0.1");
$cmd->exit_is_num(2);
$cmd->stdout_is_eq("");
$cmd->stderr_like(qr{^127\.0\.0\.1: .*(?i:name|address|host|resolv).*\n$});
}

# fping -a
{
my $cmd = Test::Command->new(cmd => "fping -a 127.0.0.1 127.0.0.2");
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("127.0.0.1\n127.0.0.2\n");
$cmd->stderr_is_eq("");
}

# fping -a --print-ttl
{
my $cmd = Test::Command->new(cmd => "fping -a --print-ttl 127.0.0.1 127.0.0.2");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr{127\.0\.0\.1 \(TTL \d+\)\n127\.0\.0\.2 \(TTL \d+\)\n});
$cmd->stderr_is_eq("");
}

# fping -a --icmp-timestamp
SKIP: {
if($^O eq 'darwin') {
    skip 'On macOS, this test is unreliable', 3;
}
my $cmd = Test::Command->new(cmd => "fping -a --icmp-timestamp 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr{127\.0\.0\.1 timestamps: Originate=\d+ Receive=\d+ Transmit=\d+ Localreceive=\d+});
$cmd->stderr_is_eq("");
}

# fping --print-ttl
{
my $cmd = Test::Command->new(cmd => "fping --print-ttl 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr{127\.0\.0\.1 is alive \(TTL \d+\)});
$cmd->stderr_is_eq("");
}

# fping --icmp-timestamp
SKIP: {
if($^O eq 'darwin') {
    skip 'On macOS, this test is unreliable', 3;
}
my $cmd = Test::Command->new(cmd => "fping --icmp-timestamp 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr{127\.0\.0\.1 is alive, timestamps: Originate=\d+ Receive=\d+ Transmit=\d+ Localreceive=\d+});
$cmd->stderr_is_eq("");
}

# fping --icmp-timestamp --print-tos --print-ttl -e
SKIP: {
if($^O eq 'darwin') {
    skip 'On macOS, this test is unreliable', 3;
}
my $cmd = Test::Command->new(cmd => "fping --icmp-timestamp --print-tos --print-ttl -e 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr{127\.0\.0\.1 is alive, timestamps: Originate=\d+ Receive=\d+ Transmit=\d+ Localreceive=\d+ \(TOS \d+\) \(TTL \d+\) \(\d+(\.\d*)? ms\)});
$cmd->stderr_is_eq("");
}

# fping --icmp-timestamp ::1
SKIP: {
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 3;
    }
    my $cmd = Test::Command->new(cmd => "fping --icmp-timestamp ::1");
    $cmd->exit_is_num(2);
    $cmd->stdout_is_eq("");
    $cmd->stderr_like(qr{^::1: .*(?i:name|address|host|resolv).*\n$});
}

# fping --print-ttl with IPv6
SKIP: {
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 3;
    }
    my $cmd = Test::Command->new(cmd => "fping --print-ttl ::1");
    $cmd->exit_is_num(0);
    $cmd->stdout_like(qr{::1 is alive \(TTL \d+\)\n});
    $cmd->stderr_is_eq("");
}

# fping -A
{
my $cmd = Test::Command->new(cmd => "fping -4 -A localhost");
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("127.0.0.1 is alive\n");
$cmd->stderr_is_eq("");
}

# fping -b
{
my $cmd = Test::Command->new(cmd => "fping -b 1000 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("127.0.0.1 is alive\n");
$cmd->stderr_is_eq("");
}

# fping -b (odd payload size with random data)
{
my $cmd = Test::Command->new(cmd => "fping -b 1 -R -q -c 5 -p 20 -t 500 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("");
$cmd->stderr_like(qr{127\.0\.0\.1 : xmt/rcv/%loss = 5/5/0%, min/avg/max = \d+(?:\.\d+)?/\d+(?:\.\d+)?/\d+(?:\.\d+)?\n});
}

# fping --icmp-timestamp -b
{
my $cmd = Test::Command->new(cmd => "fping --icmp-timestamp -b 1000 127.0.0.1");
$cmd->exit_is_num(1);
$cmd->stdout_is_eq("");
$cmd->stderr_like(qr{cannot change ICMP Timestamp size});
}

# fping -b --icmp-timestamp
{
my $cmd = Test::Command->new(cmd => "fping -b 1000 --icmp-timestamp 127.0.0.1");
$cmd->exit_is_num(1);
$cmd->stdout_is_eq("");
$cmd->stderr_like(qr{cannot change ICMP Timestamp size});
}

# fping --icmp-timestamp -b 12 (ICMP Timestamp data size)
{
my $cmd = Test::Command->new(cmd => "fping --icmp-timestamp -b 12 127.0.0.1");
$cmd->exit_is_num(1);
$cmd->stdout_is_eq("");
$cmd->stderr_like(qr{cannot change ICMP Timestamp size});
}

# fping -6 --icmp-timestamp
SKIP: {
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 3;
    }
    my $cmd = Test::Command->new(cmd => "fping -6 --icmp-timestamp ::1");
    $cmd->exit_is_num(1);
    $cmd->stdout_is_eq("");
    $cmd->stderr_like(qr{ICMP Timestamp is IPv4 only});
}

# fping --icmp-timestamp -6
SKIP: {
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 3;
    }
    my $cmd = Test::Command->new(cmd => "fping --icmp-timestamp -6 ::1");
    $cmd->exit_is_num(1);
    $cmd->stdout_is_eq("");
    $cmd->stderr_is_eq("fping: can't specify both -4 and -6\n");
}

# fping -B
SKIP: {
    if($^O eq 'darwin') {
        skip 'timing test not reliable on macOS', 5;
    }
    my $t0 = [gettimeofday];
    my $cmd = Test::Command->new(cmd => "fping  -t 100 -r 3 -B 2  8.8.8.7");
    $cmd->exit_is_num(1);
    $cmd->stdout_is_eq("8.8.8.7 is unreachable\n");
    $cmd->stderr_like(qr{^(|(8.8.8.7: error while sending ping: No route to host\n)+)$});
    my $elapsed = tv_interval($t0);
    # 0.1 + 0.2 + 0.4 + 0.8 = 1.5
    cmp_ok($elapsed, '>=', 1.5);
    cmp_ok($elapsed, '<', 1.9);
}

# fping -B (timeout event queue middle insertion)
# -t 160, -i 80, -r 1, -B 4 across 4 targets (H0..H3), with --seqmap-timeout 0
# so loopback replies expire immediately and every ping times out.
# Initial timeout is 160ms; on retry (-r 1 -B 4), backoff makes it 160*4 = 640ms:
#   t=0ms:   send H0 -> timeout H0@160                        timeout queue: [H0@160]
#   t=80ms:  send H1 -> timeout H1@240                        timeout queue: [H0@160, H1@240]
#   t=160ms: H0 times out -> retry (640ms) H0_r@800           timeout queue: [H1@240, H0_r@800]
#            send H2 -> H2@320 inserted in the middle         timeout queue: [H1@240, H2@320, H0_r@800]
#   t=240ms: H1 times out -> retry (640ms) H1_r@880; send H3 -> H3@400 inserted between H2@320 and H0_r@800
{
my $cmd = Test::Command->new(cmd => "fping --seqmap-timeout 0 -r 1 -B 4 -t 160 -i 80 127.0.0.1 127.0.0.1 127.0.0.1 127.0.0.1");
$cmd->exit_is_num(1);
$cmd->stdout_is_eq("127.0.0.1 is unreachable\n" x 4);
$cmd->stderr_is_eq("");
}

# fping -B (timeout event queue head insertion)
# -t 100, -i 160, -r 1, -B 4 across 2 targets (H0, H1).
# Initial timeout is 100ms; on retry (-r 1 -B 4), backoff makes it 100*4 = 400ms:
#   t=0ms:   send H0 -> timeout H0@100                        timeout queue: [H0@100]
#   t=100ms: H0 times out before H1 -> retry (400ms) H0_r@500 timeout queue: [H0_r@500]
#   t=160ms: send H1 -> H1@260 inserted at head (< 500)       timeout queue: [H1@260, H0_r@500]
{
my $cmd = Test::Command->new(cmd => "fping --seqmap-timeout 0 -r 1 -B 4 -t 100 -i 160 127.0.0.1 127.0.0.1");
$cmd->exit_is_num(1);
$cmd->stdout_is_eq("127.0.0.1 is unreachable\n" x 2);
$cmd->stderr_is_eq("");
}
