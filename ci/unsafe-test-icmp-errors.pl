#!/usr/bin/perl

use strict;
use warnings;
use Test::Command;
use Test::More;

if ($^O ne 'linux') {
    plan skip_all => 'Linux netns ICMP error tests only run on Linux';
    exit 0;
}
# Mutates network namespaces and sysctls; excluded from ci/test-*.pl and requires explicit opt-in.
if (($ENV{FPING_RUN_UNSAFE_TESTS} // '') ne '1') {
    plan skip_all => 'Unsafe netns test disabled; run via ci/run-tests-podman.sh or set FPING_RUN_UNSAFE_TESTS=1 in a VM/container';
    exit 0;
}
if ($> != 0) {
    plan skip_all => 'Requires root privileges (run via sudo in a VM/container)';
    exit 0;
}
if (system('ip netns list >/dev/null 2>&1') != 0) {
    plan skip_all => 'ip netns not available in this environment';
    exit 0;
}

my $ns_tag = $$ . '_' . time();
my $ns_cl = "fping_cl_$ns_tag";
my $ns_rt = "fping_rt_$ns_tag";
my ($created_cl, $created_rt) = (0, 0);

END {
    local $?;
    # Ignore termination signals while cleaning up so both netns are deleted.
    local @SIG{qw(HUP INT QUIT TERM)} = ('IGNORE') x 4;
    run_ok("ip netns del $ns_cl") if $created_cl;
    run_ok("ip netns del $ns_rt") if $created_rt;
}
# Exit via signal handler so END deletes netns on Ctrl-C / CI timeout.
for my $sig (qw(HUP INT QUIT TERM)) {
    next if ($SIG{$sig} // '') eq 'IGNORE';
    $SIG{$sig} = sub { exit 1; };
}

sub run_ok {
    note("+ $_[0]");
    return system("$_[0] >/dev/null 2>&1") == 0;
}

sub run_setup {
    note("+ $_[0]");
    my $out = `$_[0] 2>&1`;
    die "netns setup failed: $_[0]\n$out" if $? != 0;
}

if (!run_ok("ip netns add $ns_cl")) {
    plan skip_all => 'ip netns add not permitted in this environment';
    exit 0;
}
$created_cl = 1;

if (!run_ok("ip netns add $ns_rt")) {
    plan skip_all => 'ip netns add failed for router namespace';
    exit 0;
}
$created_rt = 1;

# Fall back to nsenter when ip netns exec cannot remount /sys (e.g. rootless Podman).
my $in_cl = "ip netns exec $ns_cl";
my $in_rt = "ip netns exec $ns_rt";
if (!run_ok("$in_cl true")) {
    $in_cl = "nsenter --net=/var/run/netns/$ns_cl --";
    $in_rt = "nsenter --net=/var/run/netns/$ns_rt --";
    if (!run_ok("$in_cl true")) {
        plan skip_all => 'entering network namespace not permitted in this environment';
        exit 0;
    }
}

# New netns default ip_forward=0; skip cleanly in containers with a read-only /proc/sys.
if (!run_ok("$in_rt sysctl -w net.ipv4.ip_forward=1")) {
    plan skip_all => 'cannot enable net.ipv4.ip_forward in network namespace';
    exit 0;
}

my ($ipv4_tests, $ipv6_tests) = (12, 6);
plan tests => $ipv4_tests + $ipv6_tests;

# Enable IPv6 and disable DAD before creating veths; ignore failure if IPv6 is disabled.
for my $ns ($in_cl, $in_rt) {
    run_ok("$ns sysctl -w net.ipv6.conf.all.disable_ipv6=0 net.ipv6.conf.default.disable_ipv6=0 net.ipv6.conf.all.accept_dad=0 net.ipv6.conf.default.accept_dad=0");
    run_setup("$ns ip link set lo up");
}

# Create veth_cl <-> veth_rt between client and router, plus veth_sink0/1 inside router
# (using a veth pair avoids requiring dummy.ko and keeps carrier up; arp off skips L2 resolution).
run_setup("$in_cl ip link add veth_cl type veth peer name veth_rt");
run_setup("$in_cl ip link set veth_rt netns $ns_rt");
run_setup("$in_rt ip link add veth_sink0 type veth peer name veth_sink1");

run_setup("$in_cl ip addr add 10.99.0.1/24 dev veth_cl");
run_setup("$in_rt ip addr add 10.99.0.2/24 dev veth_rt");
run_setup("$in_cl ip link set veth_cl up");
run_setup("$in_rt ip link set veth_rt up");
run_setup("$in_cl ip route add default via 10.99.0.2 dev veth_cl");

run_setup("$in_rt ip link set veth_sink0 arp off mtu 1280 up");
run_setup("$in_rt ip link set veth_sink1 arp off up");
run_ok("$in_rt sysctl -w net.ipv4.icmp_ratelimit=0");
run_setup("$in_rt ip route add 198.51.100.$_/32 dev veth_sink0") for 1 .. 3;

sub run_fping {
    my ($args) = @_;
    note("+ $in_cl fping $args");
    return Test::Command->new(cmd => "$in_cl fping $args");
}

# 1. IPv4 ICMP Time Exceeded (single target; also warms ARP for subsequent tests)
{
    my $cmd = run_fping('-4 -s -r 0 -t 500 -H 1 198.51.100.1');
    $cmd->exit_is_num(1);
    $cmd->stdout_is_eq("198.51.100.1 is unreachable\n");
    $cmd->stderr_like(qr{\AICMP Time Exceeded from 10\.99\.0\.2 for ICMP Echo sent to 198\.51\.100\.1\n.*^ +1 timeouts \(waiting for response\)\n +1 ICMP Echos sent\n +0 ICMP Echo Replies received\n +1 other ICMP received\n}ms);
}

# 2. IPv4 ICMP Time Exceeded (multi-target, exercises non-zero icmp_seq; error order is unspecified)
{
    my $cmd = run_fping('-4 -s -r 0 -t 500 -i 10 -H 1 198.51.100.1 198.51.100.2 198.51.100.3');
    $cmd->exit_is_num(1);
    $cmd->stdout_is_eq("198.51.100.1 is unreachable\n198.51.100.2 is unreachable\n198.51.100.3 is unreachable\n");
    $cmd->stderr_like(qr{\A(?=.*^ICMP Time Exceeded from 10\.99\.0\.2 for ICMP Echo sent to 198\.51\.100\.1\n)(?=.*^ICMP Time Exceeded from 10\.99\.0\.2 for ICMP Echo sent to 198\.51\.100\.2\n)(?=.*^ICMP Time Exceeded from 10\.99\.0\.2 for ICMP Echo sent to 198\.51\.100\.3\n).*^ +3 timeouts \(waiting for response\)\n +3 ICMP Echos sent\n +0 ICMP Echo Replies received\n +3 other ICMP received\n}ms);
}

# 3. IPv4 -q suppresses ICMP error warnings while still counting other ICMP received
{
    my $cmd = run_fping('-4 -q -s -r 0 -t 500 -H 1 198.51.100.1');
    $cmd->exit_is_num(1);
    $cmd->stdout_is_eq("");
    $cmd->stderr_like(qr{\A\n +1 targets\n +0 alive\n +1 unreachable\n +0 unknown addresses\n\n +1 timeouts \(waiting for response\)\n +1 ICMP Echos sent\n +0 ICMP Echo Replies received\n +1 other ICMP received\n});
}

# 4. IPv4 ICMP Destination Unreachable (Fragmentation Needed via -M and MTU 1280)
# Uses ip_forward() DF/MTU check rather than ip_error(), because Linux 5.10+ (CVE-2020-25705)
# rate-limits ip_error() via inet_peer independently of net.ipv4.icmp_ratelimit=0.
{
    my $cmd = run_fping('-4 -s -r 0 -t 500 -M -b 1400 198.51.100.1');
    $cmd->exit_is_num(1);
    $cmd->stdout_is_eq("198.51.100.1 is unreachable\n");
    $cmd->stderr_like(qr{\AICMP Unreachable \(Fragmentation Needed\) from 10\.99\.0\.2 for ICMP Echo sent to 198\.51\.100\.1\n.*^ +1 timeouts \(waiting for response\)\n +1 ICMP Echos sent\n +0 ICMP Echo Replies received\n +1 other ICMP received\n}ms);
}

# 5 & 6. IPv6 ICMPv6 Time Exceeded, Packet Too Big, No Route, Administratively Prohibited
SKIP: {
    skip 'Skip IPv6 tests', $ipv6_tests if $ENV{SKIP_IPV6};
    skip 'IPv6 not available in network namespace', $ipv6_tests unless run_ok("$in_rt test -d /proc/sys/net/ipv6");
    skip 'IPv6 disabled in network namespace', $ipv6_tests unless run_ok("$in_cl ip -6 addr add fd99::1/64 dev veth_cl nodad");
    skip 'cannot enable IPv6 forwarding in network namespace', $ipv6_tests unless run_ok("$in_rt sysctl -w net.ipv6.conf.all.forwarding=1");

    run_setup("$in_rt ip -6 addr add fd99::2/64 dev veth_rt nodad");
    run_setup("$in_cl ip -6 route add default via fd99::2 dev veth_cl");

    run_ok("$in_rt sysctl -w net.ipv6.icmp.ratelimit=0");
    run_setup("$in_rt ip -6 route add 2001:db8:99::1/128 dev veth_sink0");
    run_setup("$in_rt ip -6 route add unreachable 2001:db8:99::2/128");
    run_setup("$in_rt ip -6 route add prohibit 2001:db8:99::3/128");

    # 5. IPv6 ICMPv6 Time Exceeded (also warms NDP for Test 6)
    {
        my $cmd = run_fping('-6 -s -r 0 -t 500 -H 1 2001:db8:99::1');
        $cmd->exit_is_num(1);
        $cmd->stdout_is_eq("2001:db8:99::1 is unreachable\n");
        $cmd->stderr_like(qr{\AICMPv6 Time Exceeded \(Hop limit exceeded in transit\) from fd99::2 for ICMPv6 Echo Request sent to 2001:db8:99::1\n.*^ +1 timeouts \(waiting for response\)\n +1 ICMP Echos sent\n +0 ICMP Echo Replies received\n +1 other ICMP received\n}ms);
    }

    # 6. IPv6 ICMPv6 Packet Too Big + Destination Unreachable (No route + Administratively prohibited)
    {
        my $cmd = run_fping('-6 -s -r 0 -t 500 -i 10 -b 1400 2001:db8:99::1 2001:db8:99::2 2001:db8:99::3');
        $cmd->exit_is_num(1);
        $cmd->stdout_is_eq("2001:db8:99::1 is unreachable\n2001:db8:99::2 is unreachable\n2001:db8:99::3 is unreachable\n");
        $cmd->stderr_like(qr{\A(?=.*^ICMPv6 Packet Too Big from fd99::2 for ICMPv6 Echo Request sent to 2001:db8:99::1\n)(?=.*^ICMPv6 Destination Unreachable \(No route to destination\) from fd99::2 for ICMPv6 Echo Request sent to 2001:db8:99::2\n)(?=.*^ICMPv6 Destination Unreachable \(Communication with destination administratively prohibited\) from fd99::2 for ICMPv6 Echo Request sent to 2001:db8:99::3\n).*^ +3 timeouts \(waiting for response\)\n +3 ICMP Echos sent\n +0 ICMP Echo Replies received\n +3 other ICMP received\n}ms);
    }
}
