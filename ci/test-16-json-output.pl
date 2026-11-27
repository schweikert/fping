#!/usr/bin/perl -w

use Test::Command tests => 121;
use Test::More;
use File::Temp;
use JSON::PP qw(decode_json);

my $RE_RTT        = qr/\d+(?:\.\d+)?/;
my $RE_TS_EPOCH   = qr/[1-9]\d{9,}\.\d{5}/;
my $RE_TS_CTIME   = qr/\w+\s\w+\s+\d+\s\d{2}:\d{2}:\d{2}\s\d{4}/;
my $RE_TS_ISO     = qr/\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}[+-]\d{4}/;
my $RE_TS_RFC3339 = qr/\d{4}-\d{2}-\d{2}\s\d{2}:\d{2}:\d{2}/;

# Validate LF-framed JSONL where each line is a 1-key fping JSON object
# envelope (e.g. {"resp": {...}}). Emits 1 assertion and returns decoded docs.
sub jsonl_ok {
    local $Test::Builder::Level = $Test::Builder::Level + 1;
    my ($cmd) = @_;
    my $label = ref($cmd->{cmd}) eq 'ARRAY' ? join(' ', @{$cmd->{cmd}}) : ($cmd->{cmd} // '');
    $label =~ s/[\x00-\x1f\x7f]/ /g;
    my $stdout = $cmd->stdout_value // '';
    my @errors;
    my $ends_nl = length($stdout) > 0 && substr($stdout, -1) eq "\n";
    push @errors, 'missing trailing newline' if !$ends_nl;
    # Limit -1 preserves trailing empty fields; popping the final LF leaves extra blank lines visible.
    my @lines = split /\n/, $stdout, -1;
    pop @lines if $ends_nl;
    push @errors, 'empty output' if !@lines;
    my @docs;
    for my $i (0 .. $#lines) {
        my $line = $lines[$i];
        if ($line eq '' || substr($line, -1) eq "\r") {
            push @errors, "line " . ($i + 1) . ($line eq '' ? ': empty line' : ': trailing CR');
            next;
        }
        my ($doc, $eval_err);
        { local $@; $doc = eval { decode_json($line) }; $eval_err = $@; }
        if ($eval_err || ref($doc) ne 'HASH') {
            (my $err = $eval_err || 'not a JSON object') =~ s/\s+$//;
            push @errors, "line " . ($i + 1) . ": $err ($line)";
            next;
        }
        # Enforce fping's {"<type>": {...}} envelope to catch malformed or merged records.
        my @keys = keys %$doc;
        if (@keys != 1 || ref($doc->{$keys[0]}) ne 'HASH') {
            push @errors, "line " . ($i + 1) . ": expected 1-key JSON object envelope ($line)";
            next;
        }
        push @docs, $doc;
    }
    ok(!@errors, "$label: valid JSONL") or diag(join("\n", @errors));
    return @docs;
}

# fping -J -c 2 127.0.0.1
{
my $cmd = Test::Command->new(cmd => "fping -J -c 2 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr/^\{"resp":\s\{"host":\s"127\.0\.0\.1",\s"seq":\s0,\s"size":\s\d+,\s"rtt":\s$RE_RTT\}\}
\{"resp":\s\{"host":\s"127\.0\.0\.1",\s"seq":\s1,\s"size":\s\d+,\s"rtt":\s$RE_RTT\}\}
\{"summary":\s\{"host":\s"127\.0\.0\.1",\s"xmt":\s\d+,\s"rcv":\s\d+,\s"loss":\s\d+,\s"rttMin":\s$RE_RTT,\s"rttAvg":\s$RE_RTT,\s"rttMax":\s$RE_RTT\}\}\n?$/);
$cmd->stderr_is_eq("");
}

# fping -J -c 1 --icmp-timestamp 127.0.0.1
SKIP: {
if($^O eq 'darwin') {
    skip 'On macOS, this test is unreliable', 3;
}
my $cmd = Test::Command->new(cmd => "fping -J -c 1 --icmp-timestamp 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr/^\{"resp":\s\{"host":\s"127\.0\.0\.1",\s"seq":\s0,\s"size":\s\d+,\s"rtt":\s$RE_RTT,\s"timestamps":\s\{"originate":\s\d+,\s"receive":\s\d+,\s"transmit":\s\d+,\s"localreceive":\s\d+\}\}\}
\{"summary":\s\{"host":\s"127\.0\.0\.1",\s"xmt":\s\d+,\s"rcv":\s\d+,\s"loss":\s\d+,\s"rttMin":\s$RE_RTT,\s"rttAvg":\s$RE_RTT,\s"rttMax":\s$RE_RTT\}\}\n?$/);
$cmd->stderr_is_eq("");
}

# fping -J -c 1 --print-ttl 127.0.0.1
{
my $cmd = Test::Command->new(cmd => "fping -J -c 1 --print-ttl 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr/^\{"resp":\s\{"host":\s"127\.0\.0\.1",\s"seq":\s0,\s"size":\s\d+,\s"rtt":\s$RE_RTT,\s"ttl":\s\d+\}\}
\{"summary":\s\{"host":\s"127\.0\.0\.1",\s"xmt":\s\d+,\s"rcv":\s\d+,\s"loss":\s\d+,\s"rttMin":\s$RE_RTT,\s"rttAvg":\s$RE_RTT,\s"rttMax":\s$RE_RTT\}\}\n?$/);
$cmd->stderr_is_eq("");
}

# fping -J -c 1 --print-tos 127.0.0.1
{
my $cmd = Test::Command->new(cmd => "fping -J -c 1 --print-tos 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr/^\{"resp":\s\{"host":\s"127\.0\.0\.1",\s"seq":\s0,\s"size":\s\d+,\s"rtt":\s$RE_RTT,\s"tos":\s\d+\}\}
\{"summary":\s\{"host":\s"127\.0\.0\.1",\s"xmt":\s\d+,\s"rcv":\s\d+,\s"loss":\s\d+,\s"rttMin":\s$RE_RTT,\s"rttAvg":\s$RE_RTT,\s"rttMax":\s$RE_RTT\}\}\n?$/);
$cmd->stderr_is_eq("");
}

# fping -J -c 1 --print-ttl --print-tos 127.0.0.1
{
my $cmd = Test::Command->new(cmd => "fping -J -c 1 --print-ttl --print-tos 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr/^\{"resp":\s\{"host":\s"127\.0\.0\.1",\s"seq":\s0,\s"size":\s\d+,\s"rtt":\s$RE_RTT,\s"tos":\s\d+,\s"ttl":\s\d+\}\}
\{"summary":\s\{"host":\s"127\.0\.0\.1",\s"xmt":\s\d+,\s"rcv":\s\d+,\s"loss":\s\d+,\s"rttMin":\s$RE_RTT,\s"rttAvg":\s$RE_RTT,\s"rttMax":\s$RE_RTT\}\}\n?$/);
$cmd->stderr_is_eq("");
}

# fping -J -c 1 192.0.2.47
{
my $cmd = Test::Command->new(cmd => "fping -J -c 1 192.0.2.47");
$cmd->exit_is_num(1);
$cmd->stdout_like(qr/^\{"timeout":\s\{"host":\s"192\.0\.2\.47",\s"seq":\s\d+\}\}
\{"summary":\s\{"host":\s"192\.0\.2\.47",\s"xmt":\s\d+,\s"rcv":\s\d+,\s"loss":\s\d+\}\}\n?$/);
$cmd->stderr_is_eq("");
}

# fping -J -c 2 -D 127.0.0.1 (timestamp >= 1000000000 checks Unix epoch vs CLOCK_MONOTONIC uptime)
{
my $cmd = Test::Command->new(cmd => "fping -J -c 2 -D 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr/^\{"resp":\s\{"timestamp":\s"$RE_TS_EPOCH",\s"host":\s"127\.0\.0\.1",\s"seq":\s0,\s"size":\s\d+,\s"rtt":\s$RE_RTT\}\}
\{"resp":\s\{"timestamp":\s"$RE_TS_EPOCH",\s"host":\s"127\.0\.0\.1",\s"seq":\s1,\s"size":\s\d+,\s"rtt":\s$RE_RTT\}\}
\{"summary":\s\{"host":\s"127\.0\.0\.1",\s"xmt":\s\d+,\s"rcv":\s\d+,\s"loss":\s\d+,\s"rttMin":\s$RE_RTT,\s"rttAvg":\s$RE_RTT,\s"rttMax":\s$RE_RTT\}\}\n?$/);
$cmd->stderr_is_eq("");
}

# fping -J -c 1 -D --timestamp-format=ctime 127.0.0.1
{
my $cmd = Test::Command->new(cmd => "fping -J -c 1 -D --timestamp-format=ctime 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr/^\{"resp":\s\{"timestamp":\s"$RE_TS_CTIME",\s"host":\s"127\.0\.0\.1",\s"seq":\s0,\s"size":\s\d+,\s"rtt":\s$RE_RTT\}\}
\{"summary":\s\{"host":\s"127\.0\.0\.1",\s"xmt":\s\d+,\s"rcv":\s\d+,\s"loss":\s\d+,\s"rttMin":\s$RE_RTT,\s"rttAvg":\s$RE_RTT,\s"rttMax":\s$RE_RTT\}\}\n?$/);
$cmd->stderr_is_eq("");
}

# fping -J -c 1 -D --timestamp-format=iso 127.0.0.1
{
my $cmd = Test::Command->new(cmd => "fping -J -c 1 -D --timestamp-format=iso 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr/^\{"resp":\s\{"timestamp":\s"$RE_TS_ISO",\s"host":\s"127\.0\.0\.1",\s"seq":\s0,\s"size":\s\d+,\s"rtt":\s$RE_RTT\}\}
\{"summary":\s\{"host":\s"127\.0\.0\.1",\s"xmt":\s\d+,\s"rcv":\s\d+,\s"loss":\s\d+,\s"rttMin":\s$RE_RTT,\s"rttAvg":\s$RE_RTT,\s"rttMax":\s$RE_RTT\}\}\n?$/);
$cmd->stderr_is_eq("");
}

# fping -J -c 1 -D --timestamp-format=rfc3339 127.0.0.1
{
my $cmd = Test::Command->new(cmd => "fping -J -c 1 -D --timestamp-format=rfc3339 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr/^\{"resp":\s\{"timestamp":\s"$RE_TS_RFC3339",\s"host":\s"127\.0\.0\.1",\s"seq":\s0,\s"size":\s\d+,\s"rtt":\s$RE_RTT\}\}
\{"summary":\s\{"host":\s"127\.0\.0\.1",\s"xmt":\s\d+,\s"rcv":\s\d+,\s"loss":\s\d+,\s"rttMin":\s$RE_RTT,\s"rttAvg":\s$RE_RTT,\s"rttMax":\s$RE_RTT\}\}\n?$/);
$cmd->stderr_is_eq("");
}

# fping -J -c 1 --print-reply-dst 127.0.0.1
{
my $cmd = Test::Command->new(cmd => "fping -J -c 1 --print-reply-dst 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr/^\{"resp":\s\{"host":\s"127\.0\.0\.1",\s"seq":\s0,\s"size":\s\d+,\s"rtt":\s$RE_RTT,\s"replyDst":\s"127\.0\.0\.1"\}\}
\{"summary":\s\{"host":\s"127\.0\.0\.1",\s"xmt":\s\d+,\s"rcv":\s\d+,\s"loss":\s\d+,\s"rttMin":\s$RE_RTT,\s"rttAvg":\s$RE_RTT,\s"rttMax":\s$RE_RTT\}\}\n?$/);
$cmd->stderr_is_eq("");
}

# fping -J -c 1 --print-reply-dst ::1
SKIP: {
if ($ENV{SKIP_IPV6}) {
    skip 'Skip IPv6 tests', 3;
}
my $cmd = Test::Command->new(cmd => "fping -J -c 1 --print-reply-dst ::1");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr/^\{"resp":\s\{"host":\s"::1",\s"seq":\s0,\s"size":\s\d+,\s"rtt":\s$RE_RTT,\s"replyDst":\s"::1"\}\}
\{"summary":\s\{"host":\s"::1",\s"xmt":\s\d+,\s"rcv":\s\d+,\s"loss":\s\d+,\s"rttMin":\s$RE_RTT,\s"rttAvg":\s$RE_RTT,\s"rttMax":\s$RE_RTT\}\}\n?$/);
$cmd->stderr_is_eq("");
}

# fping -J -c 1 -q 127.0.0.1
{
my $cmd = Test::Command->new(cmd => "fping -J -c 1 -q 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr/^\{"summary":\s\{"host":\s"127\.0\.0\.1",\s"xmt":\s\d+,\s"rcv":\s\d+,\s"loss":\s\d+,\s"rttMin":\s$RE_RTT,\s"rttAvg":\s$RE_RTT,\s"rttMax":\s$RE_RTT\}\}\n?$/);
$cmd->stderr_is_eq("");
}

# fping -J -C 1 127.0.0.1
{
my $cmd = Test::Command->new(cmd => "fping -J -C 1 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr/^\{"resp":\s\{"host":\s"127\.0\.0\.1",\s"seq":\s\d+,\s"size":\s\d+,\s"rtt":\s$RE_RTT\}\}
\{"vSum":\s\{"host":\s"127\.0\.0\.1",\s"values":\s\[$RE_RTT\]\}\}\n$/);
$cmd->stderr_is_eq("");
}

# fping -J -C 1 192.0.2.47
{
my $cmd = Test::Command->new(cmd => "fping -J -C 1 192.0.2.47");
$cmd->exit_is_num(1);
$cmd->stdout_like(qr/^\{"timeout":\s\{"host":\s"192\.0\.2\.47",\s"seq":\s\d+\}\}
\{"vSum":\s\{"host":\s"192\.0\.2\.47",\s"values":\s\[null\]\}\}\n$/);
$cmd->stderr_is_eq("");
}

# fping -J -C 1 -q 127.0.0.1
{
my $cmd = Test::Command->new(cmd => "fping -J -C 1 -q 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr/^\{"vSum":\s\{"host":\s"\d+\.\d+\.\d+\.\d+",\s"values":\s\[$RE_RTT\]\}\}\n?$/);
$cmd->stderr_is_eq("");
}

# fping -J -C 1 -q 127.0.0.1 127.0.0.2
{
my $cmd = Test::Command->new(cmd => "fping -J -C 1 -q 127.0.0.1 127.0.0.2");
$cmd->exit_is_num(0);  # Both hosts are reachable in this test
$cmd->stdout_like(qr/^\{"vSum":\s\{"host":\s"\d+\.\d+\.\d+\.\d+",\s"values":\s\[$RE_RTT\]\}\}
\{"vSum":\s\{"host":\s"\d+\.\d+\.\d+\.\d+",\s"values":\s\[$RE_RTT\]\}\}\n?$/);
$cmd->stderr_is_eq("");
}

# fping -J -C3 -p 100 -q 127.0.0.1
{
my $cmd = Test::Command->new(cmd => "fping -J -C3 -p 100 -q 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr/^\{"vSum":\s\{"host":\s"\d+\.\d+\.\d+\.\d+",\s"values":\s\[$RE_RTT,\s$RE_RTT,\s$RE_RTT\]\}\}\n?$/);
$cmd->stderr_is_eq("");
}

# fping -J -c 1 -q host.invalid
{
my $cmd = Test::Command->new(cmd => "fping -J -c 1 -q host.invalid");
$cmd->exit_is_num(2);  # Exit code 2 indicates name resolution error
$cmd->stdout_like(qr/^\{"warning":\s\{"host":\s"host\.invalid",\s"message":\s"[^"]*(?i:name|address|host|resolv)[^"]*"\}\}\n$/);
$cmd->stderr_is_eq("");
}

# fping -J -c 1 -q -g 127.0.0.1/30
{
my $cmd = Test::Command->new(cmd => "fping -J -c 1 -q -g 127.0.0.1/30");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr/^\{"summary":\s\{"host":\s"127\.0\.0\.1",\s"xmt":\s\d+,\s"rcv":\s\d+,\s"loss":\s\d+,\s"rttMin":\s$RE_RTT,\s"rttAvg":\s$RE_RTT,\s"rttMax":\s$RE_RTT\}\}
\{"summary":\s\{"host":\s"127\.0\.0\.2",\s"xmt":\s\d+,\s"rcv":\s\d+,\s"loss":\s\d+,\s"rttMin":\s$RE_RTT,\s"rttAvg":\s$RE_RTT,\s"rttMax":\s$RE_RTT\}\}\n$/);
$cmd->stderr_is_eq("");
}

# fping -J -c 2 -Q 1 127.0.0.1 (time >= 1000000000 checks Unix epoch vs CLOCK_MONOTONIC uptime)
{
my $cmd = Test::Command->new(cmd => "fping -J -c 2 -Q 1 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr/^\{"intSum":\s\{"time":\s[1-9]\d{9,},"host":\s"127\.0\.0\.1",\s"xmt":\s\d+,\s"rcv":\s\d+,\s"loss":\s\d+,\s"rttMin":\s$RE_RTT,\s"rttAvg":\s$RE_RTT,\s"rttMax":\s$RE_RTT\}\}
\{"summary":\s\{"host":\s"127\.0\.0\.1",\s"xmt":\s\d+,\s"rcv":\s\d+,\s"loss":\s\d+,\s"rttMin":\s$RE_RTT,\s"rttAvg":\s$RE_RTT,\s"rttMax":\s$RE_RTT\}\}\n$/);
$cmd->stderr_is_eq("");
}

# fping -J -s -q -c 2 127.0.0.1
{
my $cmd = Test::Command->new(cmd => "fping -J -s -q -c 2 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr/^\{"summary":\s\{"host":\s"127\.0\.0\.1",\s"xmt":\s\d+,\s"rcv":\s\d+,\s"loss":\s\d+,\s"rttMin":\s$RE_RTT,\s"rttAvg":\s$RE_RTT,\s"rttMax":\s$RE_RTT\}\}
\{"stats":\s\{"targets":\s\d+,\s"alive":\s\d+,\s"unreachable":\s\d+,\s"unknownAddresses":\s\d+,\s"timeouts":\s\d+,\s"icmpEchosSent":\s\d+,\s"icmpEchoRepliesReceived":\s\d+,\s"otherIcmpReceived":\s0,\s"rttMin":\s$RE_RTT,\s"rttAvg":\s$RE_RTT,\s"rttMax":\s$RE_RTT,\s"elapsed":\s\d+\.\d+\}\}\n$/);
$cmd->stderr_is_eq("");
}

# fping -J 127.0.0.1
{
my $cmd = Test::Command->new(cmd => "fping -J 127.0.0.1");
$cmd->exit_is_num(1);
$cmd->stdout_is_eq("");
$cmd->stderr_is_eq("fping: option -J, --json requires -c, -C, or -l\n");
}

# fping -J escapes quotes, backslashes, and control characters in host warnings
{
my $bad_host = "bad\"\\slash\x01\thost.invalid";
my $cmd = Test::Command->new(cmd => ['fping', '-J', '-c', '1', '-q', $bad_host]);
$cmd->exit_is_num(2, 'JSON escaping warning exits 2');
$cmd->stdout_like(qr/\\"\\\\slash\\u0001\\thost\.invalid/, 'JSON escaping warning uses short and \\u0001 escapes');
$cmd->stderr_is_eq("", 'JSON escaping warning stderr empty');
my @docs = jsonl_ok($cmd);
is(scalar @docs, 1, 'JSON escaping warning emits 1 record');
is($docs[0]{warning}{host}, $bad_host, 'JSON escaping warning round-trips host string');
ok(length($docs[0]{warning}{message} // '') > 0, 'JSON escaping warning includes message');
}

# fping -J -x / -X are rejected as mutually exclusive
{
my $cmd = Test::Command->new(cmd => ['fping', '-J', '-c', '1', '-x', '1', '127.0.0.1']);
$cmd->exit_is_num(1, '-J -x 1 exits 1');
$cmd->stdout_is_eq("", '-J -x 1 stdout is empty');
$cmd->stderr_is_eq("fping: option -J, --json cannot be used with -x or -X\n", '-J -x 1 reports mutual exclusion on stderr');
}
{
my $cmd = Test::Command->new(cmd => ['fping', '-J', '-c', '1', '-X', '1', '127.0.0.1']);
$cmd->exit_is_num(1, '-J -X 1 exits 1');
$cmd->stdout_is_eq("", '-J -X 1 stdout is empty');
$cmd->stderr_is_eq("fping: option -J, --json cannot be used with -x or -X\n", '-J -X 1 reports mutual exclusion on stderr');
}

# fping -J -N is rejected as mutually exclusive
{
my $cmd = Test::Command->new(cmd => ['fping', '-J', '-N', '-c', '1', '127.0.0.1']);
$cmd->exit_is_num(1, '-J -N exits 1');
$cmd->stdout_is_eq("", '-J -N stdout is empty');
$cmd->stderr_is_eq("fping: specify only one of -J, -N\n", '-J -N reports mutual exclusion on stderr');
}

# fping -J --seqmap-timeout 0 -D -o -s -c 1 -t 50 127.0.0.1
{
my $cmd = Test::Command->new(cmd => "fping -J --seqmap-timeout 0 -D -o -s -c 1 -t 50 127.0.0.1");
$cmd->exit_is_num(1);
$cmd->stdout_like(qr/^\{"timeout":\s\{"timestamp":\s"$RE_TS_EPOCH",\s"host":\s"127\.0\.0\.1",\s"seq":\s0\}\}
\{"summary":\s\{"host":\s"127\.0\.0\.1",\s"xmt":\s1,\s"rcv":\s0,\s"loss":\s100,\s"outage\(ms\)":\s1000\}\}
\{"stats":\s\{"targets":\s1,\s"alive":\s0,\s"unreachable":\s1,\s"unknownAddresses":\s0,\s"timeouts":\s1,\s"icmpEchosSent":\s1,\s"icmpEchoRepliesReceived":\s0,\s"otherIcmpReceived":\s0,\s"rttMin":\s0\.000,\s"rttAvg":\s0\.000,\s"rttMax":\s0\.000,\s"elapsed":\s\d+\.\d+\}\}\n?$/);
$cmd->stderr_is_eq("");
}

# fping -J --seqmap-timeout 0 -c 2 -t 50 -Q 0.8 -p 1000 -o 127.0.0.1
SKIP: {
if($^O eq 'darwin') {
    skip 'On macOS, -Q tests are unreliable', 4;
}
my $cmd = Test::Command->new(cmd => "fping -J --seqmap-timeout 0 -c 2 -t 50 -Q 0.8 -p 1000 -o 127.0.0.1");
$cmd->exit_is_num(1);
$cmd->stdout_like(qr/^(?:\{"intSum":\s\{[^\n]+\}\}\n)*\{"intSum":\s\{"time":\s[1-9]\d{9,},"host":\s"127\.0\.0\.1",\s"xmt":\s1,\s"rcv":\s0,\s"loss":\s100,\s"outage\(ms\)":\s1000\}\}
(?:\{"intSum":\s\{[^\n]+\}\}\n)*\{"summary":\s\{"host":\s"127\.0\.0\.1",\s"xmt":\s2,\s"rcv":\s0,\s"loss":\s100,\s"outage\(ms\)":\s2000\}\}\n?$/);
$cmd->stderr_is_eq("");
jsonl_ok($cmd);
}

# fping -J -p 100 -t 1000 -l 127.0.0.1 with SIGQUIT
{
my $cmd = Test::Command->new(cmd => '(sleep 1; pkill -QUIT fping; sleep 1; pkill -INT fping)& fping -J -p 100 -t 1000 -l 127.0.0.1');
$cmd->stdout_like(qr/^(?:\{"(?:resp|timeout)":\s\{[^\n]+\}\}\n)*\{"intSum":\s\{"time":\s[1-9]\d{9,},"host":\s"127\.0\.0\.1",\s"xmt":\s\d+,\s"rcv":\s\d+,\s"loss":\s\d+(?:,\s"rttMin":\s$RE_RTT,\s"rttAvg":\s$RE_RTT,\s"rttMax":\s$RE_RTT)?\}\}
(?:\{"(?:resp|timeout)":\s\{[^\n]+\}\}\n)*\{"summary":\s\{"host":\s"127\.0\.0\.1",\s"xmt":\s\d+,\s"rcv":\s\d+,\s"loss":\s\d+(?:,\s"rttMin":\s$RE_RTT,\s"rttAvg":\s$RE_RTT,\s"rttMax":\s$RE_RTT)?\}\}\n?$/);
$cmd->stderr_is_eq("");
jsonl_ok($cmd);
}

# fping -J without -q emits both stderr warning and stdout JSON warning
{
my $warn_host = "bad\"\\host\n.invalid";
my $cmd = Test::Command->new(cmd => ['fping', '-J', '-c', '1', $warn_host]);
$cmd->exit_is_num(2, 'JSON non-quiet warning exits 2');
# No /s: \Q...\E matches $warn_host's embedded \n while [^\n]+ enforces a single error line.
$cmd->stderr_like(qr/^\Q$warn_host\E: [^\n]+\n\z/, 'JSON non-quiet warning writes stderr');
$cmd->stdout_like(qr/^\{"warning":\s\{"host":\s"bad\\"\\\\host\\n\.invalid",\s"message":\s"[^"\n]+"\}\}\n\z/, 'JSON non-quiet warning writes stdout');
my @docs = jsonl_ok($cmd);
is($docs[0]{warning}{host}, $warn_host, 'JSON non-quiet warning round-trips host');
}

# fping -J with mixed invalid and valid targets emits valid NDJSON (warning + resp + summary)
{
my $cmd = Test::Command->new(cmd => ['fping', '-J', '-c', '1', 'bad"host.invalid', '127.0.0.1']);
$cmd->exit_is_num(2);
$cmd->stderr_like(qr/^\Qbad"host.invalid\E: [^\n]+\n\z/);
$cmd->stdout_like(qr/^\{"warning":\s\{"host":\s"bad\\"host\.invalid",\s"message":\s"[^"\n]+"\}\}
\{"resp":\s\{"host":\s"127\.0\.0\.1",\s"seq":\s0,\s"size":\s\d+,\s"rtt":\s$RE_RTT\}\}
\{"summary":\s\{"host":\s"127\.0\.0\.1",\s"xmt":\s1,\s"rcv":\s1,\s"loss":\s0,\s"rttMin":\s$RE_RTT,\s"rttAvg":\s$RE_RTT,\s"rttMax":\s$RE_RTT\}\}\n\z/);
jsonl_ok($cmd);
}

# fping -J -f - reads targets from stdin and escapes warning hosts
{
my $tmp = File::Temp->new();
my $stdin_bad = "bad\"\\slash\x01\x1f\x7f.invalid";
print $tmp "$stdin_bad\n127.0.0.1\n";
close($tmp);
# POSIX single-quote the temp path since shell redirection is used for stdin.
(my $q = $tmp->filename) =~ s/'/'\\''/g;
my $cmd = Test::Command->new(cmd => "fping -J -c 1 -q -f - < '$q'");
$cmd->exit_is_num(2);
$cmd->stderr_is_eq("");
$cmd->stdout_like(qr/^\{"warning":\s\{"host":\s"bad\\"\\\\slash\\u0001\\u001f\x7f\.invalid",\s"message":\s"[^"\n]+"\}\}
\{"summary":\s\{"host":\s"127\.0\.0\.1",\s"xmt":\s1,\s"rcv":\s1,\s"loss":\s0,\s"rttMin":\s$RE_RTT,\s"rttAvg":\s$RE_RTT,\s"rttMax":\s$RE_RTT\}\}\n\z/);
my @docs = jsonl_ok($cmd);
is($docs[0]{warning}{host}, $stdin_bad, 'JSON -f - stdin warning round-trips host');
}

# fping -J -n (reverse DNS)
{
my $cmd = Test::Command->new(cmd => "fping -J -c 1 -n 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr/^\{"resp":\s\{"host":\s"([^"\s]+)",\s"seq":\s0,\s"size":\s\d+,\s"rtt":\s$RE_RTT\}\}
\{"summary":\s\{"host":\s"\1",\s"xmt":\s1,\s"rcv":\s1,\s"loss":\s0,\s"rttMin":\s$RE_RTT,\s"rttAvg":\s$RE_RTT,\s"rttMax":\s$RE_RTT\}\}\n\z/);
$cmd->stderr_is_eq("");
jsonl_ok($cmd);
}

# fping -J -A -n (reverse DNS + numeric address)
{
my $cmd = Test::Command->new(cmd => "fping -J -c 1 -A -n 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr/^\{"resp":\s\{"host":\s"([^\s"]+ \(127\.0\.0\.1\))",\s"seq":\s0,\s"size":\s\d+,\s"rtt":\s$RE_RTT\}\}
\{"summary":\s\{"host":\s"\1",\s"xmt":\s1,\s"rcv":\s1,\s"loss":\s0,\s"rttMin":\s$RE_RTT,\s"rttAvg":\s$RE_RTT,\s"rttMax":\s$RE_RTT\}\}\n\z/);
$cmd->stderr_is_eq("");
jsonl_ok($cmd);
}

# fping -J -c 1 -q -s with invalid and valid targets reports unknownAddresses == 1
{
my $cmd = Test::Command->new(cmd => "fping -J -c 1 -q -s host.invalid 127.0.0.1");
$cmd->exit_is_num(2);
$cmd->stdout_like(qr/^\{"warning":\s\{"host":\s"host\.invalid",\s"message":\s"[^"\n]+"\}\}
\{"summary":\s\{"host":\s"127\.0\.0\.1",\s"xmt":\s1,\s"rcv":\s1,\s"loss":\s0,\s"rttMin":\s$RE_RTT,\s"rttAvg":\s$RE_RTT,\s"rttMax":\s$RE_RTT\}\}
\{"stats":\s\{"targets":\s1,\s"alive":\s1,\s"unreachable":\s0,\s"unknownAddresses":\s1,\s"timeouts":\s0,\s"icmpEchosSent":\s1,\s"icmpEchoRepliesReceived":\s1,\s"otherIcmpReceived":\s0,\s"rttMin":\s$RE_RTT,\s"rttAvg":\s$RE_RTT,\s"rttMax":\s$RE_RTT,\s"elapsed":\s\d+\.\d+\}\}\n\z/);
$cmd->stderr_is_eq("");
jsonl_ok($cmd);
}
