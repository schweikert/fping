#!/usr/bin/perl -w

# regression testing for github issue #56
#
use Test::Command;
use Test::More;

if( $^O eq 'darwin' ) {
    plan skip_all => 'Test disabled on MacOS';
    exit 0;
}

plan tests => 3;

my $cmd1 = Test::Command->new(cmd => "fping -t100 -p100 -C3 255.255.255.255");
$cmd1->exit_is_num(1);
$cmd1->stdout_is_eq("");
$cmd1->stderr_is_eq("\n255.255.255.255 : - - -\n");
