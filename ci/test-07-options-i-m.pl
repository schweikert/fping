#!/usr/bin/perl -w

use Test::Command tests => 60;
use Test::More;
use Time::HiRes qw(gettimeofday tv_interval);

#  -i n       interval between sending ping packets (in millisec) (default 25)
#  -I IFACE   bind to a particular interface, not always available
#  -l         loop sending pings forever
#  -k         set fwmark on ping packets
#  -m         ping multiple interfaces on target host
#  -M         don't fragment

# fping -i n
{
my $cmd = Test::Command->new(cmd => "fping -i 100 127.0.0.1 127.0.0.2");
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("127.0.0.1 is alive\n127.0.0.2 is alive\n");
$cmd->stderr_is_eq("");
}

# fping -i n with interval > current_time_ns (4e18 ns > Unix epoch until 2096
# and > CLOCK_MONOTONIC uptime): verify the first ping is sent immediately
# rather than waiting for opt.interval - 0 as if a ping had been sent at t=0.
{
my $cmd = Test::Command->new(cmd => "fping -i 4000000000000 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("127.0.0.1 is alive\n");
$cmd->stderr_is_eq("");
}

# fping -I IFACE
SKIP: {
if($^O ne 'linux') {
    skip '-I option functionality is only tested on Linux', 3;
}
my $cmd = Test::Command->new(cmd => 'fping -I lo 127.0.0.1');
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("127.0.0.1 is alive\n");
$cmd->stderr_is_eq("");
}

# fping -I IFACE
SKIP: {
if($^O ne 'linux') {
    skip '-I option functionality is only tested on Linux', 3;
}
if($ENV{SKIP_IPV6}) {
    skip 'Skip IPv6 tests', 3;
}
my $cmd = Test::Command->new(cmd => 'fping -6 -I lo ::1');
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("::1 is alive\n");
$cmd->stderr_is_eq("");
}

# fping -I IFACE
SKIP: {
if($^O ne 'linux') {
    skip '-I option functionality is only tested on Linux', 3;
}
my $cmd = Test::Command->new(cmd => 'fping -I NotAnInterface 127.0.0.1');
$cmd->exit_is_num(1);
$cmd->stdout_is_eq("");
$cmd->stderr_like(qr{binding to specific interface \(SO_BINDTODEVICE\):.*\n});
}

# fping --oiface=IFACE
SKIP: {
if($^O ne 'linux') {
    skip '--oiface option functionality is only tested on Linux', 3;
}
my $cmd = Test::Command->new(cmd => 'fping --oiface=lo 127.0.0.1');
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("127.0.0.1 is alive\n");
$cmd->stderr_is_eq("");
}

# fping --oiface=IFACE (IPv6)
SKIP: {
    if($^O ne 'linux') {
        skip '--oiface option functionality is only tested on Linux', 3;
    }
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 3;
    }
    my $cmd = Test::Command->new(cmd => 'fping -6 --oiface=lo ::1');
    $cmd->exit_is_num(0);
    $cmd->stdout_is_eq("::1 is alive\n");
    $cmd->stderr_is_eq("");
}

# fping -l
{
my $cmd = Test::Command->new(cmd => '(sleep 0.5; pkill -INT fping)& fping -p 100 -t 2000 -l 127.0.0.1');
$cmd->stdout_like(qr{127\.0\.0\.1 : \[0\], 64 bytes, \d+\.\d+ ms \(\d+\.\d+ avg, 0% loss\)
127\.0\.0\.1 : \[1\], 64 bytes, \d+\.\d+ ms \(\d+\.\d+ avg, 0% loss\)
});
}

# fping -l --print-tos --print-ttl
{
my $cmd = Test::Command->new(cmd => '(sleep 0.5; pkill -INT fping)& fping -p 100 -t 2000 --print-ttl --print-tos -l 127.0.0.1');
$cmd->stdout_like(qr{127\.0\.0\.1 : \[0\], 64 bytes, \d+\.\d+ ms \(\d+\.\d+ avg, 0% loss\) \(TOS \d+\) \(TTL \d+\)
127\.0\.0\.1 : \[1\], 64 bytes, \d+\.\d+ ms \(\d+\.\d+ avg, 0% loss\) \(TOS \d+\) \(TTL \d+\)
});
}

# fping -k
SKIP: {
if($^O ne 'linux') {
    skip '-k option is only supported on Linux', 3;
}
my $cmd = Test::Command->new(cmd => 'fping -k 256 127.0.0.1');
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("127.0.0.1 is alive\n");
$cmd->stderr_is_eq("");
}

# fping -l with SIGQUIT
{
my $cmd = Test::Command->new(cmd => '(sleep 0.5; pkill -QUIT fping; sleep 0.5; pkill -INT fping)& fping -p 100 -t 2000 -l 127.0.0.1');
$cmd->stdout_like(qr{127\.0\.0\.1 : \[0\], 64 bytes, \d+\.\d+ ms \(\d+\.\d+ avg, 0% loss\)
127\.0\.0\.1 : \[1\], 64 bytes, \d+\.\d+ ms \(\d+\.\d+ avg, 0% loss\)
127\.0\.0\.1 : \[2\], 64 bytes, \d+\.\d+ ms \(\d+\.\d+ avg, 0% loss\)
127\.0\.0\.1 : \[3\], 64 bytes, \d+\.\d+ ms \(\d+\.\d+ avg, 0% loss\)
127\.0\.0\.1 : \[4\], 64 bytes, \d+\.\d+ ms \(\d+\.\d+ avg, 0% loss\)
});
$cmd->stderr_like(qr{\[\d+:\d+:\d+\]
127\.0\.0\.1 : xmt/rcv/%loss = \d+/\d+/\d+%, min/avg/max = \d+\.\d+/\d+\.\d+/\d+\.\d+
});
}

# fping -l -Q
SKIP: {
if($^O eq 'darwin') {
    skip 'On macOS, this test is unreliable', 2;
}
my $cmd = Test::Command->new(cmd => '(sleep 1.5; pkill -INT fping)& fping -p 100 -l -Q 0.5 127.0.0.1');
$cmd->stdout_is_eq("");
$cmd->stderr_like(qr{\[\d\d:\d\d:\d\d\]
127\.0\.0\.1 : xmt/rcv/%loss = \d/\d/\d%, min/avg/max = \d+\.\d+/\d+\.\d+/\d+\.\d+
\[\d\d:\d\d:\d\d\]
127\.0\.0\.1 : xmt/rcv/%loss = \d/\d/\d%, min/avg/max = \d+\.\d+/\d+\.\d+/\d+\.\d+
});
}

# fping -l -t
{
my $cmd = Test::Command->new(cmd => '(sleep 0.5; pkill -INT fping)& fping -p 100 -t 1500 -l 127.0.0.1');
$cmd->stdout_like(qr{127\.0\.0\.1 : \[0\], 64 bytes, \d+\.\d+ ms \(\d+\.\d+ avg, 0% loss\)
127\.0\.0\.1 : \[1\], 64 bytes, \d+\.\d+ ms \(\d+\.\d+ avg, 0% loss\)
});
}

# fping -M
SKIP: {
if($^O eq 'openbsd') {
    skip '-M option not supported on OpenBSD', 3;
}
my $cmd = Test::Command->new(cmd => "fping -M 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("127.0.0.1 is alive\n");
$cmd->stderr_is_eq("");
}

# fping --frag
SKIP: {
if($^O eq 'openbsd') {
    skip '--frag option not supported on OpenBSD', 3;
}
my $cmd = Test::Command->new(cmd => "fping --frag 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("127.0.0.1 is alive\n");
$cmd->stderr_is_eq("");
}

# fping --oiface error cases
SKIP: {
if($^O ne 'linux') {
    skip '--oiface option functionality is only tested on Linux', 9;
}
my $cmd1 = Test::Command->new(cmd => 'fping --oiface=NotAnInterface 127.0.0.1');
$cmd1->exit_is_num(1);
$cmd1->stdout_is_eq("");
$cmd1->stderr_is_eq("fping: unknown interface 'NotAnInterface'\n");

my $cmd2 = Test::Command->new(cmd => 'fping -I lo --oiface=lo 127.0.0.1');
$cmd2->exit_is_num(1);
$cmd2->stdout_is_eq("");
$cmd2->stderr_is_eq("fping: specify only --oiface or -I, --iface\n");

my $cmd3 = Test::Command->new(cmd => 'fping --oiface=NotAnInterface -I lo 127.0.0.1');
$cmd3->exit_is_num(1);
$cmd3->stdout_is_eq("");
$cmd3->stderr_is_eq("fping: specify only --oiface or -I, --iface\n");
}

# fping --oiface with -v skips interface validation
{
my $cmd = Test::Command->new(cmd => 'fping --oiface=NotAnInterface -v');
$cmd->exit_is_num(0);
$cmd->stdout_like(qr{^fping: Version \S+});
$cmd->stderr_is_eq("");
}

# fping -l with -p > -t
{
my $cmd = Test::Command->new(cmd => '(sleep 1; pkill -INT fping)& fping -p 500 -t 400 -l 127.0.0.1');
$cmd->stdout_like(qr{127\.0\.0\.1 : \[0\], 64 bytes, \d+(?:\.\d+)? ms \(\d+(?:\.\d+)? avg, 0% loss\)});
}

# fping -m
{
my $cmd = Test::Command->new(cmd => "fping -4 -m localhost");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr{^(localhost is alive\n)+$});
$cmd->stderr_is_eq("");
}

# fping -l responds promptly to SIGQUIT and SIGINT during a long select() wait
{
my $t0 = [gettimeofday];
my $cmd = Test::Command->new(cmd => '(sleep 0.5; pkill -QUIT fping; sleep 0.3; pkill -INT fping)& fping -l -p 10000 -t 10000 127.0.0.1');
$cmd->exit_is_num(0);
my $elapsed = tv_interval($t0);
cmp_ok($elapsed, '<', 4.0, 'SIGQUIT/SIGINT interrupts 10s select() wait promptly');
$cmd->stdout_like(qr{^127\.0\.0\.1 : \[0\], 64 bytes, \d+(?:\.\d+)? ms \(\d+(?:\.\d+)? avg, 0% loss\)\n$});
$cmd->stderr_like(qr{\[\d\d:\d\d:\d\d\]\n127\.0\.0\.1 : xmt/rcv/%loss = 1/1/0%[^\n]*\n\n127\.0\.0\.1 : xmt/rcv/%loss = 1/1/0%});
}

# fping -Q preserves interval statistics across SIGQUIT snapshots
SKIP: {
if($^O eq 'darwin') {
    skip 'On macOS, -Q tests are unreliable', 3;
}
my $cmd = Test::Command->new(cmd => '(sleep 0.5; pkill -QUIT fping)& fping -c 13 -p 100 -t 1000 -Q 1 127.0.0.1');
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("");
my @xmts = ($cmd->stderr_value =~ /127\.0\.0\.1 : xmt\/rcv\/%loss = (\d+)\//g);
ok(scalar(@xmts) == 3 && $xmts[0] >= 1 && $xmts[2] == 13 && ($xmts[0] + $xmts[1]) > $xmts[2],
    'SIGQUIT snapshot did not reset -Q interval counters (xmt counts: ' . join(',', @xmts) . ')');
}
