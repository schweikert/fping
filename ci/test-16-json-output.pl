#!/usr/bin/perl -w

use Test::Command tests => 18;
use Test::More;

# Test basic JSON output with a single host
{
    my $cmd = Test::Command->new(cmd => "fping -J -c 1 127.0.0.1");
    $cmd->exit_is_num(0);
    $cmd->stdout_like(qr/^\{\s*"hosts":\s*\[\s*\{\s*"host":\s*"127\.0\.0\.1"\s*,\s*"xmt":\s*"1"\s*,\s*"rcv":\s*"1"\s*,\s*"loss":\s*"0"\s*,\s*"min":\s*"\d+\.\d+"\s*,\s*"avg":\s*"\d+\.\d+"\s*,\s*"max":\s*"\d+\.\d+"\s*\}\s*\]\s*\}\n?$/, "Basic JSON structure is correct");
    $cmd->stderr_is_eq("");
}

# Test pretty-printed JSON output
{
    my $cmd = Test::Command->new(cmd => "fping -J -C 1 --json-pretty -c 1 127.0.0.1");
    $cmd->exit_is_num(0);
    $cmd->stdout_like(qr/^\{\n[\s\t]+"hosts":\s*\[\n[\s\t]+\{\n[\s\t]+"host":\s*"127\.0\.0\.1"\s*,\n[\s\t]+"count":\s*\[\s*"\d+\.\d+"\s*\]\s*\n[\s\t]+\}\n[\s\t]*\]\n\}\n?$/, "Pretty-printed JSON structure is correct");
    $cmd->stderr_is_eq("");
}

# Test JSON output with multiple hosts
{
    my $cmd = Test::Command->new(cmd => "fping -J -C 1 127.0.0.1 127.0.0.2");
    $cmd->exit_is_num(0);  # Both hosts are reachable in this test
    $cmd->stdout_like(qr/^\{\s*"hosts":\s*\[\s*\{\s*"host":\s*"127\.0\.0\.1"\s*,\s*"count":\s*\[\s*"\d+\.\d+"\s*\]\s*\}\s*,\s*\{\s*"host":\s*"127\.0\.0\.2"\s*,\s*"count":\s*\[\s*"\d+\.\d+"\s*\]\s*\}\s*\]\s*\}\n?$/, "Multiple hosts count status is correct");
    $cmd->stderr_is_eq("");
}

# Test JSON output with statistics (-C option)
{
    my $cmd = Test::Command->new(cmd => "fping -J -C3 -p 100 127.0.0.1");
    $cmd->exit_is_num(0);
    $cmd->stdout_like(qr/^\{\s*"hosts":\s*\[\s*\{\s*"host":\s*"127\.0\.0\.1"\s*,\s*"count":\s*\[\s*"\d+\.\d+"\s*,\s*"\d+\.\d+"\s*,\s*"\d+\.\d+"\s*\]\s*\}\s*\]\s*\}\n?$/, "Count-based JSON structure is correct");
    $cmd->stderr_is_eq("");
}

# Test JSON output with unreachable host
{
    my $cmd = Test::Command->new(cmd => "fping -J -c1 host.invalid");
    $cmd->exit_is_num(2);  # Exit code 2 indicates name resolution error
    $cmd->stdout_like(qr/^\{"host":\s*"host\.invalid","error":\s*"(?:Name or service not known|Temporary failure in name resolution)"\}$/, "Unreachable host JSON structure is correct");
    $cmd->stderr_is_eq("");
}

# Test JSON output with multiple hosts
{
    my $cmd = Test::Command->new(cmd => "fping -J -c 1 -g 127.0.0.1/30");
    $cmd->exit_is_num(0);
    $cmd->stdout_like(qr/^\{\s*"hosts":\s*\[\s*\{\s*"host":\s*"127\.0\.0\.1"\s*,\s*"xmt":\s*"1"\s*,\s*"rcv":\s*"1"\s*,\s*"loss":\s*"0"\s*,\s*"min":\s*"\d+\.\d+"\s*,\s*"avg":\s*"\d+\.\d+"\s*,\s*"max":\s*"\d+\.\d+"\s*\}\s*,\s*\{\s*"host":\s*"127\.0\.0\.2"\s*,\s*"xmt":\s*"1"\s*,\s*"rcv":\s*"1"\s*,\s*"loss":\s*"0"\s*,\s*"min":\s*"\d+\.\d+"\s*,\s*"avg":\s*"\d+\.\d+"\s*,\s*"max":\s*"\d+\.\d+"\s*\}\s*\]\s*\}\n?$/, "Multiple hosts JSON structure is correct");
    $cmd->stderr_is_eq("");
}
