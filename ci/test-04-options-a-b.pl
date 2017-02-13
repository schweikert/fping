#!/usr/bin/perl -w

use Test::Command tests => 32;
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
$cmd->stderr_like(qr{^::1:.*(not supported|not known)});
}

# fping -6
SKIP: {
    if(system("/sbin/ifconfig | grep inet6") != 0) {
        skip 'No IPv6 on this host', 3;
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
$cmd->stderr_like(qr{127\.0\.0\.1:.*(not supported|not known)});
}

# fping -a
{
my $cmd = Test::Command->new(cmd => "fping -a 127.0.0.1 127.0.0.2");
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("127.0.0.1\n127.0.0.2\n");
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

# fping -B
{
my $t0 = [gettimeofday];
my $cmd = Test::Command->new(cmd => "fping  -t 100 -r 3 -B 2  8.8.8.7");
$cmd->exit_is_num(1);
$cmd->stdout_is_eq("8.8.8.7 is unreachable\n");
$cmd->stderr_like(qr{^(|(8.8.8.7: error while sending ping: No route to host\n)+)$});
my $elapsed = tv_interval($t0);
# 0.1 + 0.2 + 0.4 + 0.8 = 1.5
cmp_ok($elapsed, '>=', 1.5);
cmp_ok($elapsed, '<', 1.8);
}
