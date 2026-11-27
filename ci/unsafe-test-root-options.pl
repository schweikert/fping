#!/usr/bin/perl

use strict;
use warnings;
use Test::Command;
use Test::More;

if (($ENV{FPING_RUN_UNSAFE_TESTS} // '') ne '1') {
    plan skip_all => 'Root option test disabled; run via ci/run-tests-podman.sh or set FPING_RUN_UNSAFE_TESTS=1';
    exit 0;
}
if ($< != 0) {
    plan skip_all => 'Requires real UID 0 (run via sudo in a VM/container)';
    exit 0;
}

plan tests => 3;

# As root (bypassing safe-limits 10 ms non-root floor), -p must still be >= 0.001 ms
{
    my $cmd = Test::Command->new(cmd => 'fping -p 0.0001 127.0.0.1');
    $cmd->exit_is_num(1);
    $cmd->stdout_is_eq("");
    $cmd->stderr_is_eq("fping: -p must be >= 0.001\n");
}
