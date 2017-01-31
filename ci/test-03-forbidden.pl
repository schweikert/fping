#!/usr/bin/perl -w

use Test::Command tests => 24;

# fping -i 0
my $cmd1 = Test::Command->new(cmd => "fping -i 0 -T10 -g 127.0.0.1/29");
$cmd1->exit_is_num(1);
$cmd1->stdout_is_eq("");
$cmd1->stderr_is_eq(<<END);
fping: these options are too risky for mere mortals.
fping: You need -i >= 1 and -p >= 10
END

# fping -p 9
my $cmd2 = Test::Command->new(cmd => "fping -c3 -p 9 127.0.0.1");
$cmd2->exit_is_num(1);
$cmd2->stdout_is_eq("");
$cmd2->stderr_is_eq(<<END);
fping: these options are too risky for mere mortals.
fping: You need -i >= 1 and -p >= 10
END

# fping -H 300
my $cmd5 = Test::Command->new(cmd => "fping -H 300 127.0.0.1");
$cmd5->exit_is_num(1);
$cmd5->stdout_is_eq("");
$cmd5->stderr_is_eq("fping: ttl 300 out of range\n");

# fping -a -u
my $cmd6 = Test::Command->new(cmd => "fping -a -u 127.0.0.1");
$cmd6->exit_is_num(1);
$cmd6->stdout_is_eq("");
$cmd6->stderr_is_eq("fping: specify only one of a, u\n");

# fping -c -l
my $cmd7 = Test::Command->new(cmd => "fping -c3 -l 127.0.0.1");
$cmd7->exit_is_num(1);
$cmd7->stdout_is_eq("");
$cmd7->stderr_is_eq("fping: specify only one of c, l\n");

# fping -b 65509
my $cmd8 = Test::Command->new(cmd => "fping -b 65509 127.0.0.1");
$cmd8->exit_is_num(1);
$cmd8->stdout_is_eq("");
$cmd8->stderr_is_eq("fping: data size 65509 not valid, must be lower than 65488\n");

# fping -B 0.9
my $cmd9 = Test::Command->new(cmd => "fping -B 0.9 127.0.0.1");
$cmd9->exit_is_num(1);
$cmd9->stdout_is_eq("");
$cmd9->stderr_is_eq("fping: backoff factor 0.9 not valid, must be between 1.0 and 5.0\n");

# fping -B 0.9
my $cmd10 = Test::Command->new(cmd => "fping -B 5.1 127.0.0.1");
$cmd10->exit_is_num(1);
$cmd10->stdout_is_eq("");
$cmd10->stderr_is_eq("fping: backoff factor 5.1 not valid, must be between 1.0 and 5.0\n");

