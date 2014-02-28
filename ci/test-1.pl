#!/usr/bin/perl -w

use Test::Command tests => 3;
my $fping_bin = '/opt/fping/sbin/fping';

my $cmd1 = Test::Command->new(cmd => "sudo $fping_bin 127.0.0.1");
$cmd1->exit_is_num(0);
$cmd1->stdout_is_eq("127.0.0.1 is alive\n");
$cmd1->stderr_is_eq("");
