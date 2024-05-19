#!/usr/bin/perl -w

use Test::Command tests => 27;
use Test::More;

#  -R         random bytes
#  -r n       number of retries (default 3)
#  -s         print final stats
#  -S addr    set source address
#  -t n       individual target initial timeout (in millisec) (default 500)
#  -T n       ignored (for compatibility with fping 2.4)

# fping -R
{
my $cmd = Test::Command->new(cmd => "fping -q -R -c3 -p100 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("");
$cmd->stderr_like(qr{127\.0\.0\.1 : xmt/rcv/%loss = 3/3/0%.*});
}

SKIP: {
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 3;
    }
    my $cmd = Test::Command->new(cmd => "fping -q -R -c3 -p100 ::1");
    $cmd->exit_is_num(0);
    $cmd->stdout_is_eq("");
    $cmd->stderr_like(qr{::1 : xmt/rcv/%loss = 3/3/0%.*});
}

# fping -r tested in test-4-options-a-b.pl

# fping -s
{
my $cmd = Test::Command->new(cmd => "fping -s 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("127.0.0.1 is alive\n");
$cmd->stderr_like(qr{\s*
\s*1 targets
\s*1 alive
\s*0 unreachable
\s*0 unknown addresses
\s*
\s*0 timeouts \(waiting for response\)
\s*1 ICMP Echos sent
\s*1 ICMP Echo Replies received
\s*0 other ICMP received

\s*\d\.\d+ ms \(min round trip time\)
\s*\d\.\d+ ms \(avg round trip time\)
\s*\d\.\d+ ms \(max round trip time\)
\s*\d\.\d+ sec \(elapsed real time\)
});
}

# fping -s (no host reachable)
{
my $cmd = Test::Command->new(cmd => "fping -r0 -t100 -s 8.8.0.0");
$cmd->exit_is_num(1);
$cmd->stdout_is_eq("8.8.0.0 is unreachable\n");
$cmd->stderr_like(qr{\s*
\s*1 targets
\s*0 alive
\s*1 unreachable
\s*0 unknown addresses
\s*
\s*1 timeouts \(waiting for response\)
\s*1 ICMP Echos sent
\s*0 ICMP Echo Replies received
\s*0 other ICMP received

\s*\d\.\d+ ms \(min round trip time\)
\s*\d\.\d+ ms \(avg round trip time\)
\s*\d\.\d+ ms \(max round trip time\)
\s*\d\.\d+ sec \(elapsed real time\)
});
}

# fping -S
{
my $cmd = Test::Command->new(cmd => "fping -S 127.0.0.1 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("127.0.0.1 is alive\n");
$cmd->stderr_is_eq("");
}

# fping -S (wrong source address)
{
my $cmd = Test::Command->new(cmd => "fping -S 192.0.2.47 127.0.0.1");
$cmd->exit_is_num(4);
$cmd->stdout_is_eq("");
$cmd->stderr_like(qr{fping: cannot bind source address : .+\n});
}

# fping -S
SKIP: {
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 3;
    }
    my $cmd = Test::Command->new(cmd => "fping -S ::1 ::1");
    $cmd->exit_is_num(0);
    $cmd->stdout_is_eq("::1 is alive\n");
    $cmd->stderr_is_eq("");
}

# fping -S (wrong IPv6 source address)
SKIP: {
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 3;
    }
    my $cmd = Test::Command->new(cmd => "fping -S 2001:db8::1 ::1");
    $cmd->exit_is_num(4);
    $cmd->stdout_is_eq("");
    $cmd->stderr_like(qr{fping: cannot bind source address : .+\n});
}

# fping -S
{
my $cmd = Test::Command->new(cmd => "fping -S bla");
$cmd->exit_is_num(1);
$cmd->stdout_is_eq("");
$cmd->stderr_is_eq("fping: can't parse source address: bla\n");
}

# (note: fping -t also tested in test-4-options-a-b.pl)
