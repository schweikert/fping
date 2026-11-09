#!/usr/bin/perl -w

use Test::Command tests => 371;
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

# non-ASCII high-bit byte in numeric argument must not trigger signed-char UB in isspace()
{
    my $cmd = Test::Command->new(cmd => ['fping', '-c', "\x801"]);
    $cmd->exit_is_num(1, 'fping -c high-bit byte exits 1');
    $cmd->stdout_is_eq("", 'fping -c high-bit byte stdout empty');
    $cmd->stderr_like(qr{Usage:}, 'fping -c high-bit byte prints usage');
}

# fping -g strict mask

for my $target (qw(127.0.0.1/24abc 127.0.0.1/ 127.0.0.1/abc)) {
    my $cmd = Test::Command->new(cmd => "fping -g $target");
    $cmd->exit_is_num(1);
    $cmd->stdout_is_eq("");
    $cmd->stderr_like(qr{Usage:});
}

# long-option --opt=val must dispatch by option name, not substring match in value
for my $args (
    '--seqmap-timeout=check-source -h',
    '--seqmap-timeout=icmp-timestamp -h',
    '--seqmap-timeout=print-ttl -h',
) {
    my $cmd = Test::Command->new(cmd => "fping $args");
    $cmd->exit_is_num(1);
    $cmd->stdout_is_eq("");
    $cmd->stderr_like(qr{Usage:});
}

{
    my $cmd = Test::Command->new(cmd => "fping --oiface=check-source 127.0.0.1");
    $cmd->exit_isnt_num(0);
    $cmd->stdout_is_eq("");
}

# floating-point options must reject NaN, Inf, and -0
for my $arg (qw(B i p t Q -seqmap-timeout)) {
    for my $test_input (qw(nan inf -0)) {
        my $cmd = Test::Command->new(cmd => "fping -$arg $test_input");
        $cmd->exit_is_num(1);
        $cmd->stdout_is_eq("");
        $cmd->stderr_like(qr{Usage:});
    }
}

# numeric options must reject out-of-range / overflow values
for my $args (
    '-t 1e15',
    '-i 1e15',
    '-p 1e15',
    '--seqmap-timeout=1e15',
    '-Q 0',
    '-Q 1e-10',
    '-Q 5000000000',
    '-B 1e40',
    '-b 4294967296',
    '-r 2147483647',
    '-c 2147483648',
    '-C 2147483648',
) {
    my $cmd = Test::Command->new(cmd => "fping $args");
    $cmd->exit_is_num(1);
    $cmd->stdout_is_eq("");
    $cmd->stderr_like(qr{Usage:});
}
