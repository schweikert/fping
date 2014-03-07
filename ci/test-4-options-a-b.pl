#!/usr/bin/perl -w

use Test::Command tests => 14;
use Test::More;
use Time::HiRes qw(gettimeofday tv_interval);

#  -a         show targets that are alive
#  -A         show targets by address
#  -b n       amount of ping data to send, in bytes (default 56)
#  -B f       set exponential backoff factor to f

# fping -a
my $cmd1 = Test::Command->new(cmd => "fping -a 127.0.0.1 127.0.0.2");
$cmd1->exit_is_num(0);
$cmd1->stdout_is_eq("127.0.0.1\n127.0.0.2\n");
$cmd1->stderr_is_eq("");

# fping -A
my $cmd2 = Test::Command->new(cmd => "fping -A 127.0.0.1");
$cmd2->exit_is_num(0);
$cmd2->stdout_is_eq("127.0.0.1 is alive\n");
$cmd2->stderr_is_eq("");

# fping -b
my $cmd3 = Test::Command->new(cmd => "fping -b 1000 127.0.0.1");
$cmd3->exit_is_num(0);
$cmd3->stdout_is_eq("127.0.0.1 is alive\n");
$cmd3->stderr_is_eq("");

# fping -B
my $t0 = [gettimeofday];
my $cmd4 = Test::Command->new(cmd => "fping  -t 100 -r 3 -B 2  8.8.8.7");
$cmd4->exit_is_num(1);
$cmd4->stdout_is_eq("8.8.8.7 is unreachable\n");
$cmd4->stderr_is_eq("");
my $elapsed = tv_interval($t0);
# 0.1 + 0.2 + 0.4 + 0.8 = 1.5
cmp_ok($elapsed, '>=', 1.5);
cmp_ok($elapsed, '<', 1.7);
