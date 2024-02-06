#!/usr/bin/perl -w

use Test::Command tests => 42;
use Test::More;

for my $arg (qw(b B c C H i O p Q r t x X)) {
    my $cmd = Test::Command->new(cmd => "fping -$arg xxx");
    $cmd->exit_is_num(1);
    $cmd->stdout_is_eq("");
    $cmd->stderr_like(qr{Usage:});
}

# fping -k, only supported on Linux, requires a number
SKIP: {
    if($^O ne 'linux') {
        skip '-k option is only supported on Linux', 3;
    }
    my $cmd = Test::Command->new(cmd => 'fping -k xxx 127.0.0.1');
    $cmd->exit_is_num(1);
    $cmd->stdout_is_eq("");
    $cmd->stderr_like(qr{Usage:});
}
