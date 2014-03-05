#!/usr/bin/perl -w

# regression testing for github issue #56

use Test::Command tests => 3;

my $cmd1 = Test::Command->new(cmd => "fping -t100 -p100 -c3 192.168.255.255");
$cmd1->exit_is_num(1);
$cmd1->stdout_is_eq("");
$cmd1->stderr_is_eq("\n192.168.255.255 : xmt/rcv/%loss = 3/0/100%\n");
