#!/usr/bin/perl -w

use Test::Command;
use Test::More;

plan tests => 3;

# fping
{
my $cmd = Test::Command->new(cmd => "fping -c 2 -Q 1 -N 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr{CHART fping\.127_0_0_1_packets '' 'FPing Packets' packets '127.0.0.1' fping\.packets line 110020 1
DIMENSION xmt sent absolute 1 1
DIMENSION rcv received absolute 1 1
BEGIN fping\.127_0_0_1_packets
SET xmt = 1
SET rcv = 1
END
CHART fping\.127_0_0_1_quality '' 'FPing Quality' percentage '127.0.0.1' fping\.quality area 110010 1
DIMENSION returned '' absolute 1 1
BEGIN fping\.127_0_0_1_quality
SET returned = 100
END
CHART fping\.127_0_0_1_latency '' 'FPing Latency' ms '127.0.0.1' fping\.latency area 110000 1
DIMENSION min minimum absolute 1 1000000
DIMENSION max maximum absolute 1 1000000
DIMENSION avg average absolute 1 1000000
BEGIN fping\.127_0_0_1_latency
SET min = \d+
SET avg = \d+
SET max = \d+
END}
);
$cmd->stderr_like(qr{127.0.0.1 : xmt/rcv/%loss = 2/2/0%, min/avg/max = \d\.\d+/\d\.\d+/\d\.\d+});
}
