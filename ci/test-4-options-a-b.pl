#!/usr/bin/perl -w

use Test::Command tests => 20;
use Test::More;
use Time::HiRes qw(gettimeofday tv_interval);

#  -a         show targets that are alive
#  -A         show targets by address
#  -b n       amount of ping data to send, in bytes (default 56)
#  -B f       set exponential backoff factor to f

# fping -a
{
my $cmd = Test::Command->new(cmd => "fping -a 127.0.0.1 127.0.0.2");
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("127.0.0.1\n127.0.0.2\n");
$cmd->stderr_is_eq("");
}

# fping -A
{
my $cmd = Test::Command->new(cmd => "fping -A localhost");
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("127.0.0.1 is alive\n");
$cmd->stderr_is_eq("");
}

# fping -A -n
{
my $cmd = Test::Command->new(cmd => "fping -A -n localhost");
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("localhost (127.0.0.1) is alive\n");
$cmd->stderr_is_eq("");
}

# fping6 -A -n
{
my $cmd = Test::Command->new(cmd => "fping6 -n -A 2001:4860:4860::8888");
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("google-public-dns-a.google.com (2001:4860:4860::8888) is alive\n");
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
$cmd->stderr_is_eq("");
my $elapsed = tv_interval($t0);
# 0.1 + 0.2 + 0.4 + 0.8 = 1.5
cmp_ok($elapsed, '>=', 1.5);
cmp_ok($elapsed, '<', 1.8);
}
