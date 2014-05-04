#!/usr/bin/perl -w

use Test::Command tests => 12;

#  -r n       number of retries (default 3)
#  -s         print final stats
#  -S addr    set source address
#  -t n       individual target initial timeout (in millisec) (default 500)
#  -T n       ignored (for compatibility with fping 2.4)

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

\s*0.\d+ ms \(min round trip time\)
\s*0.\d+ ms \(avg round trip time\)
\s*0.\d+ ms \(max round trip time\)
\s*0.\d+ sec \(elapsed real time\)
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

\s*0.\d+ ms \(min round trip time\)
\s*0.\d+ ms \(avg round trip time\)
\s*0.\d+ ms \(max round trip time\)
\s*0.\d+ sec \(elapsed real time\)
});
}

# fping -S
{
my $cmd = Test::Command->new(cmd => "fping -S 127.0.0.1 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("127.0.0.1 is alive\n");
$cmd->stderr_is_eq("");
}

# fping6 -S
{
my $cmd = Test::Command->new(cmd => "fping6 -S ::1 ::1");
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("::1 is alive\n");
$cmd->stderr_is_eq("");
}

# fping -t tested in test-4-options-a-b.pl
