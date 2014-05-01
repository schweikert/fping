#!/usr/bin/perl -w

use Test::Command tests => 9;
use Test::More;
use Time::HiRes qw(gettimeofday tv_interval);
use File::Temp;

#  -n         show targets by name (-d is equivalent)
#  -p n       interval between ping packets to one target (in millisec)
#               (in looping and counting modes, default 1000)
#  -q         quiet (don't show per-target/per-ping results)
#  -Q n       same as -q, but show summary every n seconds

# fping -n
{
my $cmd = Test::Command->new(cmd => "fping -n 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("localhost is alive\n");
$cmd->stderr_is_eq("");
}

# fping -q
{
my $cmd = Test::Command->new(cmd => "fping -q -p 100 -c 3 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("");
$cmd->stderr_like(qr{127\.0\.0\.1 : xmt/rcv/%loss = 3/3/0%, min/avg/max = 0\.\d+/0\.\d+/0\.\d+
});
}

# fping -Q
{
my $cmd = Test::Command->new(cmd => "fping -Q 1 -p 400 -c 4 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("");
$cmd->stderr_like(qr{\[\d+:\d+:\d+\]
127\.0\.0\.1 : xmt/rcv/%loss = 3/3/0%, min/avg/max = 0\.\d+/0\.\d+/0\.\d+
127\.0\.0\.1 : xmt/rcv/%loss = 4/4/0%, min/avg/max = 0\.\d+/0\.\d+/0\.\d+
});
}


