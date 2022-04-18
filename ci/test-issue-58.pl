#!/usr/bin/perl -w

# regression testing for github issue #58

use Test::Command tests => 3;

my $cmd1 = Test::Command->new(cmd => "fping -a -g 2001:db8:120:4161::4/64");
$cmd1->exit_is_num(1);
$cmd1->stdout_is_eq("");
$cmd1->stderr_like(qr{fping[:,] (netmask must be between 65 and 128 \(is: 64\)|-g does not support this address family|can't parse address 2001:db8:120:4161::4: Address family.*not (supported|known))\n});
