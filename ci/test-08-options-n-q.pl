#!/usr/bin/perl -w

use Test::Command tests => 30;

#  -n         show targets by name (-d is equivalent)
#  -o         show outage time (only with -c n, -C n -Q m, and -l)
#  -O n       set the type of service (tos) flag on the ICMP packets
#  -p n       interval between ping packets to one target (in millisec)
#               (in looping and counting modes, default 1000)
#  -q         quiet (don't show per-target/per-ping results)
#  -Q n       same as -q, but show summary every n seconds

# fping -n -> test-14-internet-hosts

# fping -d -n
{
my $cmd = Test::Command->new(cmd => "fping -d -n 127.0.0.1");
$cmd->exit_is_num(1);
$cmd->stdout_is_eq("");
$cmd->stderr_is_eq("fping: use either one of -d or -n\n");
}

# fping -n -d
{
my $cmd = Test::Command->new(cmd => "fping -n -d 127.0.0.1");
$cmd->exit_is_num(1);
$cmd->stdout_is_eq("");
$cmd->stderr_is_eq("fping: use either one of -d or -n\n");
}

# fping -o -c
{
my $cmd = Test::Command->new(cmd => "fping -t100 -p 100 -o -c 5 8.8.8.7");
$cmd->exit_is_num(1);
$cmd->stdout_is_eq("8.8.8.7 : [0], timed out (NaN avg, 100% loss)
8.8.8.7 : [1], timed out (NaN avg, 100% loss)
8.8.8.7 : [2], timed out (NaN avg, 100% loss)
8.8.8.7 : [3], timed out (NaN avg, 100% loss)
8.8.8.7 : [4], timed out (NaN avg, 100% loss)
");
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
$cmd->stderr_like(qr{127\.0\.0\.1 : xmt/rcv/%loss = 3/3/0%, min/avg/max = \d\.\d+/\d\.\d+/\d\.\d+
});
}

# fping -q -o -c
{
my $cmd = Test::Command->new(cmd => "fping -q -p 100 -c 3 -o 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("");
$cmd->stderr_like(qr{127\.0\.0\.1 : xmt/rcv/%loss = 3/3/0%, outage\(ms\) = 0, min/avg/max = \d\.\d+/\d\.\d+/\d\.\d+
});
}

# fping -Q -o -C
{
my $cmd = Test::Command->new(cmd => "fping -Q0.18 -p100 -C3 -o 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("");
$cmd->stderr_like(qr{\[\d{2}:\d{2}:\d{2}\]
127\.0\.0\.1 : xmt/rcv/%loss = 2/2/0%, outage\(ms\) = 0, min/avg/max = \d\.\d+/\d\.\d+/\d\.\d+
127\.0\.0\.1 :( \d+.\d+){3}
});
}

# fping -Q
{
my $cmd = Test::Command->new(cmd => "fping -Q 1 -p 400 -c 4 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("");
$cmd->stderr_like(qr{\[\d+:\d+:\d+\]
127\.0\.0\.1 : xmt/rcv/%loss = 3/3/0%, min/avg/max = \d\.\d+/\d\.\d+/\d\.\d+
127\.0\.0\.1 : xmt/rcv/%loss = 4/4/0%, min/avg/max = \d\.\d+/\d\.\d+/\d\.\d+
});
}

# fping -Q (longer test to show two time stamps and reset statistics)
{
my $cmd = Test::Command->new(cmd => "fping -Q 1 -p 550 -c 5 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("");
$cmd->stderr_like(qr{\[\d+:\d+:\d+\]
127\.0\.0\.1 : xmt/rcv/%loss = 2/2/0%, min/avg/max = \d\.\d+/\d\.\d+/\d\.\d+
\[\d+:\d+:\d+\]
127\.0\.0\.1 : xmt/rcv/%loss = 2/2/0%, min/avg/max = \d\.\d+/\d\.\d+/\d\.\d+
127\.0\.0\.1 : xmt/rcv/%loss = 5/5/0%, min/avg/max = \d\.\d+/\d\.\d+/\d\.\d+
});
}

# fping -Q -o
{
my $cmd = Test::Command->new(cmd => "fping -c4 -Q1 -p550 -o 8.8.8.7");
$cmd->exit_is_num(1);
$cmd->stdout_is_eq("");
$cmd->stderr_like(qr{\[\d+:\d+:\d+\]
8\.8\.8\.7 : xmt/rcv/%loss = 1/0/100%, outage\(ms\) = 55\d
\[\d+:\d+:\d+\]
8\.8\.8\.7 : xmt/rcv/%loss = 2/0/100%, outage\(ms\) = 110\d
8\.8\.8\.7 : xmt/rcv/%loss = 4/0/100%, outage\(ms\) = 220\d
});
}

