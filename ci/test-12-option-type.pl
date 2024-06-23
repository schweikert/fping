#!/usr/bin/perl -w

use Test::Command tests => 84;
use Test::More;

# some options require a numeric argument
for my $arg (qw(b B c C H i O p Q r t x X)) {
    for my $test_input (qw(xxx '')) {
        my $cmd = Test::Command->new(cmd => "fping -$arg $test_input");
        $cmd->exit_is_num(1);
        $cmd->stdout_is_eq("");
        $cmd->stderr_like(qr{Usage:});
    }
}

# fping -k, only supported on Linux, requires a number
SKIP: {
    if($^O ne 'linux') {
        skip '-k option is only supported on Linux', 6;
    }
    for my $test_input (qw(xxx '')) {
        my $cmd = Test::Command->new(cmd => "fping -k $test_input 127.0.0.1");
        $cmd->exit_is_num(1);
        $cmd->stdout_is_eq("");
        $cmd->stderr_like(qr{Usage:});
    }
}
