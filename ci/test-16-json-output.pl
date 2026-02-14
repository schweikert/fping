#!/usr/bin/perl -w

use Test::Command tests => 66;
use Test::More;

# fping -J -c 2 127.0.0.1
{
my $cmd = Test::Command->new(cmd => "fping -J -c 2 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr/^\{"resp":\s\{"host":\s"127\.0\.0\.1",\s"seq":\s0,\s"size":\s\d+,\s"rtt":\s\d+\.\d+\}\}
\{"resp":\s\{"host":\s"127\.0\.0\.1",\s"seq":\s1,\s"size":\s\d+,\s"rtt":\s\d+\.\d+\}\}
\{"summary":\s\{"host":\s"127\.0\.0\.1",\s"xmt":\s\d+,\s"rcv":\s\d+,\s"loss":\s\d+,\s"rttMin":\s\d+\.\d+,\s"rttAvg":\s\d+\.\d+,\s"rttMax":\s\d+\.\d+\}\}\n?$/);
$cmd->stderr_is_eq("");
}

# fping -J -c 1 --icmp-timestamp 127.0.0.1
SKIP: {
if($^O eq 'darwin') {
    skip 'On macOS, this test is unreliable', 3;
}
my $cmd = Test::Command->new(cmd => "fping -J -c 1 --icmp-timestamp 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr/^\{"resp":\s\{"host":\s"127\.0\.0\.1",\s"seq":\s0,\s"size":\s\d+,\s"rtt":\s\d+\.\d+,\s"timestamps":\s\{"originate":\s\d+,\s"receive":\s\d+,\s"transmit":\s\d+,\s"localreceive":\s\d+\}\}\}
\{"summary":\s\{"host":\s"127\.0\.0\.1",\s"xmt":\s\d+,\s"rcv":\s\d+,\s"loss":\s\d+,\s"rttMin":\s\d+\.\d+,\s"rttAvg":\s\d+\.\d+,\s"rttMax":\s\d+\.\d+\}\}\n?$/);
$cmd->stderr_is_eq("");
}

# fping -J -c 1 --print-ttl 127.0.0.1
{
my $cmd = Test::Command->new(cmd => "fping -J -c 1 --print-ttl 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr/^\{"resp":\s\{"host":\s"127\.0\.0\.1",\s"seq":\s0,\s"size":\s\d+,\s"rtt":\s\d+\.\d+,\s"ttl":\s\d+\}\}
\{"summary":\s\{"host":\s"127\.0\.0\.1",\s"xmt":\s\d+,\s"rcv":\s\d+,\s"loss":\s\d+,\s"rttMin":\s\d+\.\d+,\s"rttAvg":\s\d+\.\d+,\s"rttMax":\s\d+\.\d+\}\}\n?$/);
$cmd->stderr_is_eq("");
}

# fping -J -c 1 --print-tos 127.0.0.1
{
my $cmd = Test::Command->new(cmd => "fping -J -c 1 --print-tos 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr/^\{"resp":\s\{"host":\s"127\.0\.0\.1",\s"seq":\s0,\s"size":\s\d+,\s"rtt":\s\d+\.\d+,\s"tos":\s\d+\}\}
\{"summary":\s\{"host":\s"127\.0\.0\.1",\s"xmt":\s\d+,\s"rcv":\s\d+,\s"loss":\s\d+,\s"rttMin":\s\d+\.\d+,\s"rttAvg":\s\d+\.\d+,\s"rttMax":\s\d+\.\d+\}\}\n?$/);
$cmd->stderr_is_eq("");
}

# fping -J -c 1 --print-ttl --print-tos 127.0.0.1
{
my $cmd = Test::Command->new(cmd => "fping -J -c 1 --print-ttl --print-tos 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr/^\{"resp":\s\{"host":\s"127\.0\.0\.1",\s"seq":\s0,\s"size":\s\d+,\s"rtt":\s\d+\.\d+,\s"tos":\s\d+,\s"ttl":\s\d+\}\}
\{"summary":\s\{"host":\s"127\.0\.0\.1",\s"xmt":\s\d+,\s"rcv":\s\d+,\s"loss":\s\d+,\s"rttMin":\s\d+\.\d+,\s"rttAvg":\s\d+\.\d+,\s"rttMax":\s\d+\.\d+\}\}\n?$/);
$cmd->stderr_is_eq("");
}

# fping -J -c 1 192.0.2.47
{
my $cmd = Test::Command->new(cmd => "fping -J -c 1 192.0.2.47");
$cmd->exit_is_num(1);
$cmd->stdout_like(qr/^\{"timeout":\s\{"host":\s"192\.0\.2\.47",\s"seq":\s\d+\}\}
\{"summary":\s\{"host":\s"192\.0\.2\.47",\s"xmt":\s\d+,\s"rcv":\s\d+,\s"loss":\s\d+\}\}\n?$/);
$cmd->stderr_is_eq("");
}

# fping -J -c 2 -D 127.0.0.1
{
my $cmd = Test::Command->new(cmd => "fping -J -c 2 -D 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr/^\{"resp":\s\{"timestamp":\s"\d+.\d+",\s"host":\s"127\.0\.0\.1",\s"seq":\s0,\s"size":\s\d+,\s"rtt":\s\d+\.\d+\}\}
\{"resp":\s\{"timestamp":\s"\d+.\d+",\s"host":\s"127\.0\.0\.1",\s"seq":\s1,\s"size":\s\d+,\s"rtt":\s\d+\.\d+\}\}
\{"summary":\s\{"host":\s"127\.0\.0\.1",\s"xmt":\s\d+,\s"rcv":\s\d+,\s"loss":\s\d+,\s"rttMin":\s\d+\.\d+,\s"rttAvg":\s\d+\.\d+,\s"rttMax":\s\d+\.\d+\}\}\n?$/);
$cmd->stderr_is_eq("");
}

# fping -J -c 1 -D --timestamp-format=ctime 127.0.0.1
{
my $cmd = Test::Command->new(cmd => "fping -J -c 1 -D --timestamp-format=ctime 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr/^\{"resp":\s\{"timestamp":\s"\w+\s\w+\s+\d+\s[\d+:]+\s\d+",\s"host":\s"127\.0\.0\.1",\s"seq":\s0,\s"size":\s\d+,\s"rtt":\s\d+\.\d+\}\}
\{"summary":\s\{"host":\s"127\.0\.0\.1",\s"xmt":\s\d+,\s"rcv":\s\d+,\s"loss":\s\d+,\s"rttMin":\s\d+\.\d+,\s"rttAvg":\s\d+\.\d+,\s"rttMax":\s\d+\.\d+\}\}\n?$/);
$cmd->stderr_is_eq("");
}

# fping -J -c 1 -D --timestamp-format=iso 127.0.0.1
{
my $cmd = Test::Command->new(cmd => "fping -J -c 1 -D --timestamp-format=iso 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr/^\{"resp":\s\{"timestamp":\s"[\d+-]+T[\d+:]+\+\d+",\s"host":\s"127\.0\.0\.1",\s"seq":\s0,\s"size":\s\d+,\s"rtt":\s\d+\.\d+\}\}
\{"summary":\s\{"host":\s"127\.0\.0\.1",\s"xmt":\s\d+,\s"rcv":\s\d+,\s"loss":\s\d+,\s"rttMin":\s\d+\.\d+,\s"rttAvg":\s\d+\.\d+,\s"rttMax":\s\d+\.\d+\}\}\n?$/);
$cmd->stderr_is_eq("");
}

# fping -J -c 1 -D --timestamp-format=rfc3339 127.0.0.1
{
my $cmd = Test::Command->new(cmd => "fping -J -c 1 -D --timestamp-format=rfc3339 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr/^\{"resp":\s\{"timestamp":\s"[\d+-]+\s[\d+:]+",\s"host":\s"127\.0\.0\.1",\s"seq":\s0,\s"size":\s\d+,\s"rtt":\s\d+\.\d+\}\}
\{"summary":\s\{"host":\s"127\.0\.0\.1",\s"xmt":\s\d+,\s"rcv":\s\d+,\s"loss":\s\d+,\s"rttMin":\s\d+\.\d+,\s"rttAvg":\s\d+\.\d+,\s"rttMax":\s\d+\.\d+\}\}\n?$/);
$cmd->stderr_is_eq("");
}

# fping -J -c 1 --print-srcaddr 127.0.0.1
my $cmd = Test::Command->new(cmd => "fping -J -c 1 --print-srcaddr 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr/^\{"resp":\s\{"host":\s"127\.0\.0\.1",\s"seq":\s0,\s"size":\s\d+,\s"rtt":\s\d+\.\d+,\s"src":\s"\d+\.\d+\.\d+\.\d+"\}\}
\{"summary":\s\{"host":\s"127\.0\.0\.1",\s"xmt":\s\d+,\s"rcv":\s\d+,\s"loss":\s\d+,\s"rttMin":\s\d+\.\d+,\s"rttAvg":\s\d+\.\d+,\s"rttMax":\s\d+\.\d+\}\}\n?$/);
$cmd->stderr_is_eq("");

# fping -J -c 1 -q 127.0.0.1
{
my $cmd = Test::Command->new(cmd => "fping -J -c 1 -q 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr/^\{"summary":\s\{"host":\s"127\.0\.0\.1",\s"xmt":\s\d+,\s"rcv":\s\d+,\s"loss":\s\d+,\s"rttMin":\s\d+\.\d+,\s"rttAvg":\s\d+\.\d+,\s"rttMax":\s\d+\.\d+\}\}\n?$/);
$cmd->stderr_is_eq("");
}

# fping -J -C 1 127.0.0.1
{
my $cmd = Test::Command->new(cmd => "fping -J -C 1 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr/^\{"resp":\s\{"host":\s"127\.0\.0\.1",\s"seq":\s\d+,\s"size":\s\d+,\s"rtt":\s\d+.\d+\}\}
\{"vSum":\s\{"host":\s"127\.0\.0\.1",\s"values":\s\[\d+.\d+\]\}\}?$/);
$cmd->stderr_is_eq("");
}

# fping -J -C 1 192.0.2.47
{
my $cmd = Test::Command->new(cmd => "fping -J -C 1 192.0.2.47");
$cmd->exit_is_num(1);
$cmd->stdout_like(qr/^\{"timeout":\s\{"host":\s"192\.0\.2\.47",\s"seq":\s\d+\}\}
\{"vSum":\s\{"host":\s"192\.0\.2\.47",\s"values":\s\[null\]\}\}?$/);
$cmd->stderr_is_eq("");
}

# fping -J -C 1 -q 127.0.0.1
{
my $cmd = Test::Command->new(cmd => "fping -J -C 1 -q 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr/^\{"vSum":\s\{"host":\s"\d+\.\d+\.\d+\.\d+",\s"values":\s\[\d+\.\d+\]\}\}\n?$/);
$cmd->stderr_is_eq("");
}

# fping -J -C 1 -q 127.0.0.1 127.0.0.2
{
my $cmd = Test::Command->new(cmd => "fping -J -C 1 -q 127.0.0.1 127.0.0.2");
$cmd->exit_is_num(0);  # Both hosts are reachable in this test
$cmd->stdout_like(qr/^\{"vSum":\s\{"host":\s"\d+\.\d+\.\d+\.\d+",\s"values":\s\[\d+\.\d+\]\}\}
\{"vSum":\s\{"host":\s"\d+\.\d+\.\d+\.\d+",\s"values":\s\[\d+\.\d+\]\}\}\n?$/);
$cmd->stderr_is_eq("");
}

# fping -J -C3 -p 100 -q 127.0.0.1
{
my $cmd = Test::Command->new(cmd => "fping -J -C3 -p 100 -q 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr/^\{"vSum":\s\{"host":\s"\d+\.\d+\.\d+\.\d+",\s"values":\s\[\d+\.\d+\,\s\d+\.\d+,\s\d+\.\d+\]\}\}\n?$/);
$cmd->stderr_is_eq("");
}

# fping -J -c 1 -q host.invalid
{
my $cmd = Test::Command->new(cmd => "fping -J -c 1 -q host.invalid");
$cmd->exit_is_num(2);  # Exit code 2 indicates name resolution error
$cmd->stdout_like(qr/^\{"warning":\s\{"host":\s"host\.invalid",\s"message":\s"(?:Name or service not known|Temporary failure in name resolution|nodename nor servname provided, or not known)"\}\}\n$/);
$cmd->stderr_is_eq("");
}

# fping -J -c 1 -q -g 127.0.0.1/30
{
my $cmd = Test::Command->new(cmd => "fping -J -c 1 -q -g 127.0.0.1/30");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr/^\{"summary":\s\{"host":\s"127\.0\.0\.1",\s"xmt":\s\d+,\s"rcv":\s\d+,\s"loss":\s\d+,\s"rttMin":\s\d+\.\d+,\s"rttAvg":\s\d+\.\d+,\s"rttMax":\s\d+\.\d+\}\}
\{"summary":\s\{"host":\s"127\.0\.0\.2",\s"xmt":\s\d+,\s"rcv":\s\d+,\s"loss":\s\d+,\s"rttMin":\s\d+\.\d+,\s"rttAvg":\s\d+\.\d+,\s"rttMax":\s\d+\.\d+\}\}?$/);
$cmd->stderr_is_eq("");
}

# fping -J -c 2 -Q 1 127.0.0.1
{
my $cmd = Test::Command->new(cmd => "fping -J -c 2 -Q 1 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr/^\{"intSum":\s\{"time":\s\d+,"host":\s"127\.0\.0\.1",\s"xmt":\s\d+,\s"rcv":\s\d+,\s"loss":\s\d+,\s"rttMin":\s\d+\.\d+,\s"rttAvg":\s\d+\.\d+,\s"rttMax":\s\d+\.\d+\}\}
\{"summary":\s\{"host":\s"127\.0\.0\.1",\s"xmt":\s\d+,\s"rcv":\s\d+,\s"loss":\s\d+,\s"rttMin":\s\d+\.\d+,\s"rttAvg":\s\d+\.\d+,\s"rttMax":\s\d+\.\d+\}\}?$/);
$cmd->stderr_is_eq("");
}

# fping -J -s -q -c 2 127.0.0.1
{
my $cmd = Test::Command->new(cmd => "fping -J -s -q -c 2 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr/^\{"summary":\s\{"host":\s"127\.0\.0\.1",\s"xmt":\s\d+,\s"rcv":\s\d+,\s"loss":\s\d+,\s"rttMin":\s\d+\.\d+,\s"rttAvg":\s\d+\.\d+,\s"rttMax":\s\d+\.\d+\}\}
\{"stats":\s\{"targets":\s\d+,\s"alive":\s\d+,\s"unreachable":\s\d+,\s"unknownAddresses":\s\d+,\s"timeouts":\s\d+,\s"icmpEchosSent":\s\d+,\s"icmpEchoRepliesReceived":\s\d+,\s"otherIcmpReceived":\s0,\s"rttMin":\s\d+\.\d+,\s"rttAvg":\s\d+\.\d+,\s"rttMax":\s\d+\.\d+,\s"elapsed":\s\d+\.\d+\}\}?$/);
$cmd->stderr_is_eq("");
}

# fping -J 127.0.0.1
{
my $cmd = Test::Command->new(cmd => "fping -J 127.0.0.1");
$cmd->exit_is_num(1);
$cmd->stdout_is_eq("");
$cmd->stderr_is_eq("fping: option -J, --json requires -c, -C, or -l\n");
}
