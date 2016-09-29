#!/usr/bin/perl -w

use Test::Command tests => 12;

#  -n         show targets by name (-d is equivalent)
#  -O n       set the type of service (tos) flag on the ICMP packets
#  -p n       interval between ping packets to one target (in millisec)
#               (in looping and counting modes, default 1000)
#  -q         quiet (don't show per-target/per-ping results)
#  -Q n       same as -q, but show summary every n seconds

# fping -n -> test-14-internet-hosts

# fping -o
{
my $cmd = Test::Command->new(cmd => "fping -t100 -p 100 -o -c 5 8.8.8.7");
$cmd->exit_is_num(1);
$cmd->stdout_is_eq("");
$cmd->stderr_like(qr{^\s*8\.8\.8\.7 : xmt/rcv/%loss = 5/0/100%, outage\(ms\) = 50\d\s*$});
}

# fping -O
{
my $cmd = Test::Command->new(cmd => "fping -O 2 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("127.0.0.1 is alive\n");
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


