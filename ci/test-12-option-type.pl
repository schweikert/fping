#!/usr/bin/perl -w

use Test::Command tests => 267;
use Test::More;

# some options require a numeric argument
for my $arg (qw(b B c C H i O p Q r t x X -seqmap-timeout)) {
    for my $test_input ('xxx', "''", '-2', '" -2"') {
        my $cmd = Test::Command->new(cmd => "fping -$arg $test_input");
        $cmd->exit_is_num(1);
        $cmd->stdout_is_eq("");
        $cmd->stderr_like(qr{Usage:});
    }
}

# options that use strtoul and must be strict (no trailing garbage, no zero)
for my $arg (qw(c C H x X)) {
    for my $test_input (qw(10abc 0)) {
        my $cmd = Test::Command->new(cmd => "fping -$arg $test_input");
        $cmd->exit_is_num(1);
        $cmd->stdout_is_eq("");
        $cmd->stderr_like(qr{Usage:});
    }
}

# options that use strtoul and must be strict (no trailing garbage)
for my $arg (qw(r b)) {
    for my $test_input (qw(10abc)) {
        my $cmd = Test::Command->new(cmd => "fping -$arg $test_input");
        $cmd->exit_is_num(1);
        $cmd->stdout_is_eq("");
        $cmd->stderr_like(qr{Usage:});
    }
}

# options that use strtod and must be strict (no trailing garbage)
for my $arg (qw(B i p t Q -seqmap-timeout)) {
    for my $test_input (qw(1.5abc 10abc)) {
        my $cmd = Test::Command->new(cmd => "fping -$arg $test_input");
        $cmd->exit_is_num(1);
        $cmd->stdout_is_eq("");
        $cmd->stderr_like(qr{Usage:});
    }
}

# fping -k, only supported on Linux, requires a number
SKIP: {
    if($^O ne 'linux') {
        skip '-k option is only supported on Linux', 12;
    }
    for my $test_input ('xxx', "''", '-2', '" -2"') {
        my $cmd = Test::Command->new(cmd => "fping -k $test_input 127.0.0.1");
        $cmd->exit_is_num(1);
        $cmd->stdout_is_eq("");
        $cmd->stderr_like(qr{Usage:});
    }
}

# fping -k strictness
SKIP: {
    if($^O ne 'linux') {
        skip '-k option is only supported on Linux', 6;
    }
    my $arg = 'k';
    for my $test_input (qw(10abc 0)) {
        my $cmd = Test::Command->new(cmd => "fping -$arg $test_input 127.0.0.1");
        $cmd->exit_is_num(1);
        $cmd->stdout_is_eq("");
        $cmd->stderr_like(qr{Usage:});
    }
}

# fping -g strict mask

for my $target (qw(127.0.0.1/24abc 127.0.0.1/ 127.0.0.1/abc)) {
    my $cmd = Test::Command->new(cmd => "fping -g $target");
    $cmd->exit_is_num(1);
    $cmd->stdout_is_eq("");
    $cmd->stderr_like(qr{Usage:});
}
