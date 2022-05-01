#!/usr/bin/perl -w

use Test::Command tests => 135;
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

# fping -4 -g (range)
{
my $cmd = Test::Command->new(cmd => "fping -4 -g 127.0.0.1 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("127.0.0.1 is alive\n");
$cmd->stderr_is_eq("");
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

# fping -6 -g (IPv6 address family with one address in IPv6 range)
SKIP: {
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 3;
    }
    my $cmd = Test::Command->new(cmd => "fping -6 -g ::1 ::1");
    $cmd->exit_is_num(0);
    $cmd->stdout_is_eq("::1 is alive\n");
    $cmd->stderr_is_eq("");
}

# fping -g (IPv6 range - low value in lower 64 bits)
SKIP: {
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 3;
    }
    my $cmd = Test::Command->new(cmd => "fping -g ::1 ::1");
    $cmd->exit_is_num(0);
    $cmd->stdout_is_eq("::1 is alive\n");
    $cmd->stderr_is_eq("");
}

# fping -g (IPv6 range - low values in lower 64 bits)
SKIP: {
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 3;
    }
    my $cmd = Test::Command->new(cmd => "fping -t 100 -r 0 -g fe80::1 fe80::2");
    $cmd->stdout_like(qr{fe80::1 is (alive|unreachable)\n});
    $cmd->stdout_like(qr{fe80::2 is (alive|unreachable)\n});
    $cmd->stderr_is_eq("");
}

# fping -g (IPv6 range - high value in lower 64 bits)
SKIP: {
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 2;
    }
    my $cmd = Test::Command->new(cmd => "fping -t 100 -r 0 -g fe80::ffff:ffff:ffff:ffff fe80::ffff:ffff:ffff:ffff");
    $cmd->stdout_like(qr{fe80::ffff:ffff:ffff:ffff is (alive|unreachable)\n});
    $cmd->stderr_is_eq("");
}

# fping -g (IPv6 range - high values in lower 64 bits)
SKIP: {
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 3;
    }
    my $cmd = Test::Command->new(cmd => "fping -t 100 -r 0 -g fe80::ffff:ffff:ffff:fffe fe80::ffff:ffff:ffff:ffff");
    $cmd->stdout_like(qr{fe80::ffff:ffff:ffff:fffe is (alive|unreachable)\n});
    $cmd->stdout_like(qr{fe80::ffff:ffff:ffff:ffff is (alive|unreachable)\n});
    $cmd->stderr_is_eq("");
}

# fping -g (IPv6 range - higher and lower 64 bits in use)
SKIP: {
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 2;
    }
    my $cmd = Test::Command->new(cmd => "fping -t 100 -r 0 -g fe80::ffff:ffff:ffff:ffff fe80:0:0:1::");
    $cmd->stdout_like(qr{fe80::ffff:ffff:ffff:ffff is (alive|unreachable)\n});
    $cmd->stdout_like(qr{fe80:0:0:1:: is (alive|unreachable)\n});
}

# fping -g (empty IPv6 range)
SKIP: {
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 3;
    }
    my $cmd = Test::Command->new(cmd => "fping -g fe80::2 fe80::1");
    $cmd->exit_is_num(1);
    $cmd->stdout_is_eq("");
    $cmd->stderr_is_eq("");
}

# fping -g (too large IPv6 range - in most significant 64 bits)
SKIP: {
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 3;
    }
    my $cmd = Test::Command->new(cmd => "fping -g fe80:: fe80:0:1::");
    $cmd->exit_is_num(1);
    $cmd->stdout_is_eq("");
    $cmd->stderr_is_eq("fping: -g parameter generates too many addresses\n");
}

# fping -g (too large IPv6 range - in least significant 64 bits)
SKIP: {
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 3;
    }
    my $cmd = Test::Command->new(cmd => "fping -g fe80:: fe80::1:0:0");
    $cmd->exit_is_num(1);
    $cmd->stdout_is_eq("");
    $cmd->stderr_is_eq("fping: -g parameter generates too many addresses\n");
}

# fping -g (too large IPv6 range - need to check both 64 bit parts)
SKIP: {
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 3;
    }
    my $cmd = Test::Command->new(cmd => "fping -g fe80::ffff:ffff:fff0:0 fe80:0:0:1::");
    $cmd->exit_is_num(1);
    $cmd->stdout_is_eq("");
    $cmd->stderr_is_eq("fping: -g parameter generates too many addresses\n");
}

# fping -g (range - mixed address families: start IPv4, end IPv6)
SKIP: {
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 3;
    }
    my $cmd = Test::Command->new(cmd => "fping -g 127.0.0.1 ::1");
    $cmd->exit_is_num(1);
    $cmd->stdout_is_eq("");
    $cmd->stderr_like(qr{can't parse address ::1:.*(not supported|not known)});
}

# fping -g (range - mixed address families: start IPv6, end IPv4)
SKIP: {
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 3;
    }
    my $cmd = Test::Command->new(cmd => "fping -g ::1 127.0.0.1");
    $cmd->exit_is_num(1);
    $cmd->stdout_is_eq("");
    $cmd->stderr_like(qr{can't parse address 127\.0\.0\.1:.*(not supported|not known)});
}

# fping -g (no scoped IPv6 addresses in range - both)
SKIP: {
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 3;
    }
    my $cmd = Test::Command->new(cmd => "fping -g fe80::1%eth42 fe80::2%eth42");
    $cmd->exit_is_num(1);
    $cmd->stdout_is_eq("");
    $cmd->stderr_is_eq("fping: -g does not support scoped IPv6 addresses\n");
}

# fping -g (no scoped IPv6 addresses in range - start)
SKIP: {
    if($ENV{SKIP_IPV6}) {
         skip 'Skip IPv6 tests', 3;
    }
    my $cmd = Test::Command->new(cmd => "fping -g fe80::1%eth42 fe80::2");
    $cmd->exit_is_num(1);
    $cmd->stdout_is_eq("");
    $cmd->stderr_is_eq("fping: -g does not support scoped IPv6 addresses\n");
}

# fping -g (no scoped IPv6 addresses in range - end)
SKIP: {
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 3;
    }
    my $cmd = Test::Command->new(cmd => "fping -g fe80::1 fe80::2%eth42");
    $cmd->exit_is_num(1);
    $cmd->stdout_is_eq("");
    $cmd->stderr_is_eq("fping: -g does not support scoped IPv6 addresses\n");
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
my $cmd = Test::Command->new(cmd => "fping -g 127.0.0.2/8");
$cmd->exit_is_num(1);
$cmd->stdout_is_eq("");
$cmd->stderr_is_eq("fping: -g parameter generates too many addresses\n");
}

# fping -4 -g (cidr)
{
my $cmd = Test::Command->new(cmd => "fping -4 -g 127.0.0.1/32");
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("127.0.0.1 is alive\n");
$cmd->stderr_is_eq("");
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

# fping -g (cidr IPv6)
SKIP: {
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 3;
    }
    my $cmd = Test::Command->new(cmd => "fping -g ::1/128");
    $cmd->exit_is_num(0);
    $cmd->stdout_is_eq("::1 is alive\n");
    $cmd->stderr_is_eq("");
}

# fping -6 -g (cidr IPv6)
SKIP: {
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 3;
    }
    my $cmd = Test::Command->new(cmd => "fping -6 -g ::1/128");
    $cmd->exit_is_num(0);
    $cmd->stdout_is_eq("::1 is alive\n");
    $cmd->stderr_is_eq("");
}

# fping -g (cidr IPv6)
SKIP: {
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 3;
    }
    my $cmd = Test::Command->new(cmd => "fping -t 100 -r 0 -g fe80::/127");
    $cmd->stdout_like(qr{fe80:: is (alive|unreachable)\n});
    $cmd->stdout_like(qr{fe80::1 is (alive|unreachable)\n});
    $cmd->stderr_is_eq("");
}

# fping -g (cidr IPv6)
SKIP: {
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 5;
    }
    my $cmd = Test::Command->new(cmd => "fping -t 100 -r 0 -g fe80::cafe:1:2:30/126");
    $cmd->stdout_like(qr{fe80::cafe:1:2:30 is (alive|unreachable)\n});
    $cmd->stdout_like(qr{fe80::cafe:1:2:31 is (alive|unreachable)\n});
    $cmd->stdout_like(qr{fe80::cafe:1:2:32 is (alive|unreachable)\n});
    $cmd->stdout_like(qr{fe80::cafe:1:2:33 is (alive|unreachable)\n});
    $cmd->stderr_is_eq("");
}

# fping -g (too short cidr IPv6 prefix)
SKIP: {
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 3;
    }
    my $cmd = Test::Command->new(cmd => "fping -g fe80::/64");
    $cmd->exit_is_num(1);
    $cmd->stdout_is_eq("");
    $cmd->stderr_is_eq("fping: netmask must be between 65 and 128 (is: 64)\n");
}

# fping -g (too long cidr IPv6 prefix)
SKIP: {
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 3;
    }
    my $cmd = Test::Command->new(cmd => "fping -g ::1/129");
    $cmd->exit_is_num(1);
    $cmd->stdout_is_eq("");
    $cmd->stderr_is_eq("fping: netmask must be between 65 and 128 (is: 129)\n");
}

# fping -g (too many addresses in cidr IPv6 prefix)
SKIP: {
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 3;
    }
    my $cmd = Test::Command->new(cmd => "fping -g fe80::/65");
    $cmd->exit_is_num(1);
    $cmd->stdout_is_eq("");
    $cmd->stderr_is_eq("fping: -g parameter generates too many addresses\n");
}

# fping -g (no scoped IPv6 addresses in cidr IPv6 prefix)
SKIP: {
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 3;
    }
    my $cmd = Test::Command->new(cmd => "fping -g fe80::1%eth42/128");
    $cmd->exit_is_num(1);
    $cmd->stdout_is_eq("");
    $cmd->stderr_is_eq("fping: -g does not support scoped IPv6 addresses\n");
}

# fping -H
{
my $cmd = Test::Command->new(cmd => "fping -H 1 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("127.0.0.1 is alive\n");
$cmd->stderr_is_eq("");
}
