#!/usr/bin/perl -w

use Test::Command tests => 146;
use Test::More;
use File::Temp;

#  -f file    read list of targets from a file ( - means stdin) (only if no -g specified)
#  -g         generate target list (only if no -f specified)
#               (specify the start and end IP in the target list, or supply a IP netmask)
#               (ex. ../src/fping -g 192.168.1.0 192.168.1.255 or ../src/fping -g 192.168.1.0/24)
#  -H n       Set the IP TTL value (Time To Live hops)

my $tmpfile = File::Temp->new();
print $tmpfile "127.0.0.1\n127.0.0.2\n";
close($tmpfile);

my $tmpfile2 = File::Temp->new();
print $tmpfile2 "# comment\n127.0.0.1\n\n127.0.0.2\n";
close($tmpfile2);

# fping without option (-> equivalent to 'fping -f -')
{
my $cmd = Test::Command->new(cmd => "cat ".$tmpfile->filename." | fping");
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("127.0.0.1 is alive\n127.0.0.2 is alive\n");
$cmd->stderr_is_eq("");
}

# fping -f -
{
my $cmd = Test::Command->new(cmd => "cat ".$tmpfile->filename." | fping -f -");
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("127.0.0.1 is alive\n127.0.0.2 is alive\n");
$cmd->stderr_is_eq("");
}

# fping -f file
{
my $cmd = Test::Command->new(cmd => "fping -f ".$tmpfile->filename);
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("127.0.0.1 is alive\n127.0.0.2 is alive\n");
$cmd->stderr_is_eq("");
}

# fping -f file (with comment and empty line)
{
my $cmd = Test::Command->new(cmd => "fping -f ".$tmpfile2->filename);
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("127.0.0.1 is alive\n127.0.0.2 is alive\n");
$cmd->stderr_is_eq("");
}

# fping -f non-existing-file (error)
{
my $cmd = Test::Command->new(cmd => "fping -f file-does-not-exist");
$cmd->exit_is_num(4);
$cmd->stdout_is_eq("");
$cmd->stderr_like(qr{: fopen :});
}

# fping -g (error: no argument)
{
my $cmd = Test::Command->new(cmd => "fping -g");
$cmd->exit_is_num(1);
$cmd->stdout_is_eq("");
$cmd->stderr_like(qr{^Usage: fping \[options\] \[targets\.\.\.\]});
}

# fping -g (error: single argument, but not in cidr format)
{
my $cmd = Test::Command->new(cmd => "fping -g 127.0.0.1");
$cmd->exit_is_num(1);
$cmd->stdout_is_eq("");
$cmd->stderr_like(qr{^Usage: fping \[options\] \[targets\.\.\.\]});
}

# fping -g (error: CIDR network is not an IP address)
{
my $cmd = Test::Command->new(cmd => "fping -g xxx/32");
$cmd->exit_is_num(1);
$cmd->stdout_is_eq("");
$cmd->stderr_like(qr{can't parse address xxx});
}

# fping -g (error: start of range is not an IP address)
{
my $cmd = Test::Command->new(cmd => "fping -g xxx 127.0.0.1");
$cmd->exit_is_num(1);
$cmd->stdout_is_eq("");
$cmd->stderr_like(qr{can't parse address xxx});
}

# fping -g (error: end of range is not an IP address)
{
my $cmd = Test::Command->new(cmd => "fping -g 127.0.0.1 yyy");
$cmd->exit_is_num(1);
$cmd->stdout_is_eq("");
$cmd->stderr_like(qr{can't parse address yyy});
}

# fping -g (error: too many arguments)
{
my $cmd = Test::Command->new(cmd => "fping -g 127.0.0.1 127.0.0.2 127.0.0.3");
$cmd->exit_is_num(1);
$cmd->stdout_is_eq("");
$cmd->stderr_like(qr{^Usage: fping \[options\] \[targets\.\.\.\]});
}

# fping -g (range)
{
my $cmd = Test::Command->new(cmd => "fping -g 127.0.0.1 127.0.0.5");
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("127.0.0.1 is alive\n127.0.0.2 is alive\n127.0.0.3 is alive\n127.0.0.4 is alive\n127.0.0.5 is alive\n");
$cmd->stderr_is_eq("");
}

# fping -4 -g (range)
{
my $cmd = Test::Command->new(cmd => "fping -4 -g 127.0.0.1 127.0.0.5");
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("127.0.0.1 is alive\n127.0.0.2 is alive\n127.0.0.3 is alive\n127.0.0.4 is alive\n127.0.0.5 is alive\n");
$cmd->stderr_is_eq("");
}

# fping -g (empty range)
{
my $cmd = Test::Command->new(cmd => "fping -g 127.0.0.2 127.0.0.1");
$cmd->exit_is_num(1);
$cmd->stdout_is_eq("");
$cmd->stderr_is_eq("");
}

# fping -g (too large range)
{
my $cmd = Test::Command->new(cmd => "fping -g 127.0.0.1 127.255.255.254");
$cmd->exit_is_num(1);
$cmd->stdout_is_eq("");
$cmd->stderr_is_eq("fping: -g parameter generates too many addresses\n");
}

# fping -g (cidr)
{
my $cmd = Test::Command->new(cmd => "fping -g 127.0.0.1/30");
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("127.0.0.1 is alive\n127.0.0.2 is alive\n");
$cmd->stderr_is_eq("");
}

# fping -g (cidr - long prefixes: point-to-point)
{
my $cmd = Test::Command->new(cmd => "fping -g 127.0.0.2/31");
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("127.0.0.2 is alive\n127.0.0.3 is alive\n");
$cmd->stderr_is_eq("");
}

# fping -g (cidr - long prefixes: host)
{
my $cmd = Test::Command->new(cmd => "fping -g 127.0.0.2/32");
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("127.0.0.2 is alive\n");
$cmd->stderr_is_eq("");
}

# fping -g (cidr - too long prefixes)
{
my $cmd = Test::Command->new(cmd => "fping -g 127.0.0.2/33");
$cmd->exit_is_num(1);
$cmd->stdout_is_eq("");
$cmd->stderr_is_eq("fping: netmask must be between 1 and 32 (is: 33)\n");
}

# fping -g (cidr - too short prefixes)
{
my $cmd = Test::Command->new(cmd => "fping -g 127.0.0.2/0");
$cmd->exit_is_num(1);
$cmd->stdout_is_eq("");
$cmd->stderr_is_eq("fping: netmask must be between 1 and 32 (is: 0)\n");
}

# fping -g (cidr - too many addresses)
{
my $cmd = Test::Command->new(cmd => "fping -g 127.0.0.0/8");
$cmd->exit_is_num(1);
$cmd->stdout_is_eq("");
$cmd->stderr_is_eq("fping: -g parameter generates too many addresses\n");
}

# fping -4 -g (range, wrong address family)
SKIP: {
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 3;
    }
    my $cmd = Test::Command->new(cmd => "fping -4 -g ::1 ::1");
    $cmd->exit_is_num(1);
    $cmd->stdout_is_eq("");
    $cmd->stderr_like(qr{can't parse address ::1:.*(not supported|not known)});
}

# fping -6 -g (range, wrong address family)
SKIP: {
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 3;
    }
    my $cmd = Test::Command->new(cmd => "fping -6 -g 127.0.0.1 127.0.0.1");
    $cmd->exit_is_num(1);
    $cmd->stdout_is_eq("");
    $cmd->stderr_like(qr{can't parse address 127\.0\.0\.1:.*(not supported|not known)});
}

# fping -g (range - IPv6)
SKIP: {
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 3;
    }
    my $cmd = Test::Command->new(cmd => "fping -g ::1 ::1");
    $cmd->exit_is_num(0);
    $cmd->stdout_is_eq("::1 is alive\n");
    $cmd->stderr_is_eq("");
}

# fping -g (empty range - IPv6)
SKIP: {
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 3;
    }
    my $cmd = Test::Command->new(cmd => "fping -g ::1 ::");
    $cmd->exit_is_num(1);
    $cmd->stdout_is_eq("");
    $cmd->stderr_is_eq("");
}

# fping -g (empty range - IPv6 - crossing 64 bit boundary)
SKIP: {
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 2;
    }
    my $cmd = Test::Command->new(cmd => "fping -g 2001:db8:0:2:: 2001:db8:0:1:ffff:ffff:ffff:ffff");
    $cmd->exit_is_num(1);
    $cmd->stdout_is_eq("");
}

# fping -6 -g (range - IPv6)
SKIP: {
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 3;
    }
    my $cmd = Test::Command->new(cmd => "fping -6 -g ::1 ::1");
    $cmd->exit_is_num(0);
    $cmd->stdout_is_eq("::1 is alive\n");
    $cmd->stderr_is_eq("");
}

# fping -g (range - scoped IPv6)
SKIP: {
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 1;
    }
    my $cmd = Test::Command->new(cmd => "fping -i1 -t10 -r0 -g fe80::47%2 fe80::48%2");
    $cmd->stdout_like(qr{fe80::47%2 is (alive|unreachable)\nfe80::48%2 is (alive|unreachable)\n});
}

# fping -g (range - scoped IPv6 - only start address is scoped)
SKIP: {
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 3;
    }
    my $cmd = Test::Command->new(cmd => "fping -i1 -t10 -r0 -g fe80::47%2 fe80::48");
    $cmd->exit_is_num(1);
    $cmd->stdout_is_eq("");
    $cmd->stderr_is_eq("fping: different scopes for start and end addresses\n");
}

# fping -g (range - scoped IPv6 - only end address is scoped)
SKIP: {
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 3;
    }
    my $cmd = Test::Command->new(cmd => "fping -i1 -t10 -r0 -g fe80::47 fe80::48%2");
    $cmd->exit_is_num(1);
    $cmd->stdout_is_eq("");
    $cmd->stderr_is_eq("fping: different scopes for start and end addresses\n");
}

# fping -g (range - inconsistently scoped IPv6)
SKIP: {
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 3;
    }
    my $cmd = Test::Command->new(cmd => "fping -i1 -t10 -r0 -g fe80::47%2 fe80::48%3");
    $cmd->exit_is_num(1);
    $cmd->stdout_is_eq("");
    $cmd->stderr_is_eq("fping: different scopes for start and end addresses\n");
}

# fping -g (range - unreachable documentation addresses)
SKIP: {
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 2;
    }
    my $cmd = Test::Command->new(cmd => "fping -t10 -r0 -i1 -g 2001:db8:1:2:3:4:5:6 2001:db8:1:2:3:4:5:7");
    $cmd->exit_is_num(1);
    $cmd->stdout_is_eq("2001:db8:1:2:3:4:5:6 is unreachable\n2001:db8:1:2:3:4:5:7 is unreachable\n");
}

# fping -g (range - unreachable documentation addresses - crossing 64 bit boundary)
SKIP: {
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 2;
    }
    my $cmd = Test::Command->new(cmd => "fping -t10 -i1 -r0 -g 2001:db8:1:2:ffff:ffff:ffff:fffe 2001:db8:1:3::1");
    $cmd->exit_is_num(1);
    $cmd->stdout_is_eq("2001:db8:1:2:ffff:ffff:ffff:fffe is unreachable
2001:db8:1:2:ffff:ffff:ffff:ffff is unreachable
2001:db8:1:3:: is unreachable
2001:db8:1:3::1 is unreachable\n");
}

# fping -g (range - too many addresses - lower 64 bit)
SKIP: {
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 3;
    }
    my $cmd = Test::Command->new(cmd => "fping -t10 -i1 -r0 -g 2001:db8:1:2:3:4:0:1 2001:db8:1:2:3:4:ff:ffff");
    $cmd->exit_is_num(1);
    $cmd->stdout_is_eq("");
    $cmd->stderr_is_eq("fping: -g parameter generates too many addresses\n");
}

# fping -g (range - too many addresses - upper 64 bit)
SKIP: {
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 3;
    }
    my $cmd = Test::Command->new(cmd => "fping -t10 -i1 -r0 -g 2001:db8:1:2::1 2001:db8:1:3::1");
    $cmd->exit_is_num(1);
    $cmd->stdout_is_eq("");
    $cmd->stderr_is_eq("fping: -g parameter generates too many addresses\n");
}

# fping -6 -g (range - mixed address families - start address IPv6)
SKIP: {
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 3;
    }
    my $cmd = Test::Command->new(cmd => "fping -6 -g ::1 127.0.0.1");
    $cmd->exit_is_num(1);
    $cmd->stdout_is_eq("");
    $cmd->stderr_like(qr{fping: can't parse address 127\.0\.0\.1: .*\n});
}

# fping -g (range - mixed address families - end address IPv6)
SKIP: {
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 3;
    }
    my $cmd = Test::Command->new(cmd => "fping -g 127.0.0.1 ::1");
    $cmd->exit_is_num(1);
    $cmd->stdout_is_eq("");
    $cmd->stderr_like(qr{fping: can't parse address ::1: .*\n});
}

# fping -6 -g (range - mixed address families - end address IPv6)
SKIP: {
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 3;
    }
    my $cmd = Test::Command->new(cmd => "fping -6 -g 127.0.0.1 ::1");
    $cmd->exit_is_num(1);
    $cmd->stdout_is_eq("");
    $cmd->stderr_like(qr{fping: can't parse address 127\.0\.0\.1: .*\n});
}

# fping -4 -g (cidr, wrong address family)
SKIP: {
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 3;
    }
    my $cmd = Test::Command->new(cmd => "fping -4 -g ::1/128");
    $cmd->exit_is_num(1);
    $cmd->stdout_is_eq("");
    $cmd->stderr_like(qr{can't parse address ::1:.*(not supported|not known)});
}

# fping -6 -g (cidr, wrong address family)
SKIP: {
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 3;
    }
    my $cmd = Test::Command->new(cmd => "fping -6 -g 127.0.0.1/32");
    $cmd->exit_is_num(1);
    $cmd->stdout_is_eq("");
    $cmd->stderr_like(qr{can't parse address 127\.0\.0\.1:.*(not supported|not known)});
}

# fping -g (CIDR - IPv6)
SKIP: {
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 3;
    }
    my $cmd = Test::Command->new(cmd => "fping -g ::1/128");
    $cmd->exit_is_num(0);
    $cmd->stdout_is_eq("::1 is alive\n");
    $cmd->stderr_is_eq("");
}

# fping -6 -g (CIDR - IPv6)
SKIP: {
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 3;
    }
    my $cmd = Test::Command->new(cmd => "fping -6 -g ::1/128");
    $cmd->exit_is_num(0);
    $cmd->stdout_is_eq("::1 is alive\n");
    $cmd->stderr_is_eq("");
}

# fping -g (CIDR - scoped IPv6)
SKIP: {
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 1;
    }
    my $cmd = Test::Command->new(cmd => "fping -t10 -r0 -g fe80::4:3:2:1%2/128");
    $cmd->stdout_like(qr{fe80::4:3:2:1%2 is (alive|unreachable)\n});
}

# fping -g (CIDR - scoped IPv6 - wrong syntax)
SKIP: {
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 3;
    }
    my $cmd = Test::Command->new(cmd => "fping -t10 -r0 -g fe80::4:3:2:1/128%2");
    $cmd->exit_is_num(1);
    $cmd->stdout_is_eq("");
    $cmd->stderr_is_eq("fping: address scope must precede prefix length\n");
}

# fping -g (CIDR - IPv6 - unreachable documentation addresses - host prefix)
SKIP: {
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 2;
    }
    my $cmd = Test::Command->new(cmd => "fping -t10 -r0 -g 2001:db8:abcd:1234:5678:9098:7654:4321/128");
    $cmd->exit_is_num(1);
    $cmd->stdout_is_eq("2001:db8:abcd:1234:5678:9098:7654:4321 is unreachable\n");
}

# fping -g (CIDR - IPv6 - unreachable documentation addresses - point-to-point prefix)
SKIP: {
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 2;
    }
    my $cmd = Test::Command->new(cmd => "fping -t10 -r0 -g 2001:db8:abcd:1234:5678:9098:7654:4320/127");
    $cmd->exit_is_num(1);
    $cmd->stdout_is_eq("2001:db8:abcd:1234:5678:9098:7654:4320 is unreachable\n2001:db8:abcd:1234:5678:9098:7654:4321 is unreachable\n");
}

# fping -g (CIDR - IPv6 - unreachable documentation addresses - normal prefix)
SKIP: {
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 2;
    }
    my $cmd = Test::Command->new(cmd => "fping -t10 -i1 -r0 -g 2001:db8:abcd:1234:5678:9098:7654:4320/126");
    $cmd->exit_is_num(1);
    $cmd->stdout_is_eq("2001:db8:abcd:1234:5678:9098:7654:4320 is unreachable
2001:db8:abcd:1234:5678:9098:7654:4321 is unreachable
2001:db8:abcd:1234:5678:9098:7654:4322 is unreachable
2001:db8:abcd:1234:5678:9098:7654:4323 is unreachable\n");
}

# fping -g (CIDR - IPv6 - prefix too short)
SKIP: {
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 3;
    }
    my $cmd = Test::Command->new(cmd => "fping -g 2001:db8::/64");
    $cmd->exit_is_num(1);
    $cmd->stdout_is_eq("");
    $cmd->stderr_is_eq("fping: netmask must be between 65 and 128 (is: 64)\n");
}

# fping -g (CIDR - IPv6 - too many addresses)
SKIP: {
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 3;
    }
    my $cmd = Test::Command->new(cmd => "fping -g 2001:db8::/65");
    $cmd->exit_is_num(1);
    $cmd->stdout_is_eq("");
    $cmd->stderr_is_eq("fping: -g parameter generates too many addresses\n");
}

# fping -g (CIDR - IPv6 - too many addresses)
SKIP: {
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 3;
    }
    my $cmd = Test::Command->new(cmd => "fping -g 2001:db8::/104");
    $cmd->exit_is_num(1);
    $cmd->stdout_is_eq("");
    $cmd->stderr_is_eq("fping: -g parameter generates too many addresses\n");
}

# fping -g (CIDR - IPv6 - prefix too long)
SKIP: {
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 3;
    }
    my $cmd = Test::Command->new(cmd => "fping -g 2001:db8::/129");
    $cmd->exit_is_num(1);
    $cmd->stdout_is_eq("");
    $cmd->stderr_is_eq("fping: netmask must be between 65 and 128 (is: 129)\n");
}

# fping -H
{
my $cmd = Test::Command->new(cmd => "fping -H 1 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("127.0.0.1 is alive\n");
$cmd->stderr_is_eq("");
}
