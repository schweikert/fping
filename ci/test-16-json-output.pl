#!/usr/bin/perl -w

use Test::Command tests => 20;
use Test::More;
use JSON;

# Test basic JSON output with a single host
{
    my $cmd = Test::Command->new(cmd => "fping -J -c 1 127.0.0.1");
    $cmd->exit_is_num(0);
    my $json = decode_json($cmd->stdout_get);
    my $host = $json->{hosts}[0];
    is($host->{host}, "127.0.0.1", "Host name is correct");
    ok(defined $host->{xmt}, "Transmitted count present");
    ok(defined $host->{rcv}, "Received count present");
    ok(defined $host->{loss}, "Loss percentage present");
    ok(defined $host->{min}, "Min time present");
    ok(defined $host->{avg}, "Avg time present");
    ok(defined $host->{max}, "Max time present");
    $cmd->stderr_is_eq("");
}

# Test pretty-printed JSON output
{
    my $cmd = Test::Command->new(cmd => "fping -J --json-pretty -c 1 127.0.0.1");
    $cmd->exit_is_num(0);
    my $output = $cmd->stdout_get;
    like($output, qr/^\{\n\s+"hosts":\s+\[\n\s+\{/, "Pretty-printed JSON has proper formatting");
    like($output, qr/\n\s{2,}"host":\s+"127\.0\.0\.1"/, "Pretty-printed JSON has proper indentation");
    my $json = decode_json($output);
    my $host = $json->{hosts}[0];
    is($host->{host}, "127.0.0.1", "Host name is correct");
    ok(defined $host->{xmt}, "Transmitted count present");
    ok(defined $host->{rcv}, "Received count present");
    $cmd->stderr_is_eq("");
}

# Test JSON output with multiple hosts
{
    my $cmd = Test::Command->new(cmd => "fping -j 127.0.0.1 127.0.0.2");
    $cmd->exit_is_num(1);  # At least one host is unreachable
    my $json = decode_json($cmd->stdout_get);
    is(@{$json->{hosts}}, 2, "JSON contains two hosts");
    is($json->{hosts}[0]{host}, "127.0.0.1", "First host is 127.0.0.1");
    is($json->{hosts}[1]{host}, "127.0.0.2", "Second host is 127.0.0.2");
    is($json->{hosts}[0]{alive}, JSON::true, "First host is alive");
    is($json->{hosts}[1]{alive}, JSON::false, "Second host is not alive");
    $cmd->stderr_is_eq("");
}

# Test JSON output with statistics (-C option)
{
    my $cmd = Test::Command->new(cmd => "fping -J -C3 -p 100 127.0.0.1");
    $cmd->exit_is_num(0);
    my $json = decode_json($cmd->stdout_get);
    my $host = $json->{hosts}[0];
    is($host->{host}, "127.0.0.1", "Host is correct");
    ok(defined $host->{count}, "Count array is present");
    is(@{$host->{count}}, 3, "Three responses recorded");
    foreach my $resp (@{$host->{count}}) {
        like($resp, qr/^\d+\.\d+$|^-$/, "Response time format is correct");
    }
    $cmd->stderr_is_eq("");
}

# Test JSON output with unreachable host
{
    my $cmd = Test::Command->new(cmd => "fping -j host.invalid");
    $cmd->exit_is_num(1);
    my $json = decode_json($cmd->stdout_get);
    is(@{$json->{hosts}}, 1, "JSON contains one host");
    is($json->{hosts}[0]{host}, "host.invalid", "Host name is correct");
    is($json->{hosts}[0]{alive}, JSON::false, "Host is not alive");
    ok(defined $json->{hosts}[0]{error}, "Error message is present");
    $cmd->stderr_is_eq("");
}

# Test JSON output with multiple hosts
{
    my $cmd = Test::Command->new(cmd => "fping -J -c 1 -g 127.0.0.1/30");
    $cmd->exit_is_num(0);
    my $json = decode_json($cmd->stdout_get);
    is(@{$json->{hosts}}, 2, "Two hosts in output");
    foreach my $host (@{$json->{hosts}}) {
        like($host->{host}, qr/^127\.0\.0\.[12]$/, "Host IP is in correct range");
        is($host->{xmt}, "1", "Transmitted count is 1");
        is($host->{rcv}, "1", "Received count is 1");
        is($host->{loss}, "0", "No packet loss");
        like($host->{min}, qr/^\d+\.\d+$/, "Min time is present");
        like($host->{avg}, qr/^\d+\.\d+$/, "Avg time is present");
        like($host->{max}, qr/^\d+\.\d+$/, "Max time is present");
    }
    $cmd->stderr_is_eq("");
}
