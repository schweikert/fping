#!/usr/bin/perl -w

use Test::Command;
use Test::More;

# evaluate if we have internet
if(!gethostbyname("www.google.com")) {
    plan skip_all => 'Can\'t resolve www.google.com -> no internet?';
    exit 0;
}

plan tests => 30;

my $re_num = qr{\d+(?:\.\d+)?};

# fping
{
my $cmd = Test::Command->new(cmd => "fping -q -i 10 -p 20 -c 3 -t200 1.1.1.1 8.8.8.8 9.9.9.9 www.france-telecom.fr www.google.com");
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("");
$cmd->stderr_like(qr{1\.1\.1\.1\s*: xmt/rcv/%loss = [123]/[123]/\d+%, min/avg/max = $re_num/$re_num/$re_num
8\.8\.8\.8\s*: xmt/rcv/%loss = [123]/[123]/\d+%, min/avg/max = $re_num/$re_num/$re_num
9\.9\.9\.9\s*: xmt/rcv/%loss = [123]/[123]/\d+%, min/avg/max = $re_num/$re_num/$re_num
www\.france-telecom\.fr\s*: xmt/rcv/%loss = [123]/[123]/\d+%, min/avg/max = $re_num/$re_num/$re_num
www\.google\.com\s*: xmt/rcv/%loss = [123]/[123]/\d+%, min/avg/max = $re_num/$re_num/$re_num
});
}

# fping -A -n
{
my $cmd = Test::Command->new(cmd => "fping -A -n 8.8.8.8");
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("dns.google (8.8.8.8) is alive\n");
$cmd->stderr_is_eq("");
}

# fping -4 -A -n
{
my $cmd = Test::Command->new(cmd => "fping -4 -A -n dns.google");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr{^dns.google \(8\.8\.(4\.4|8\.8)\) is alive\n$});
$cmd->stderr_is_eq("");
}

# fping -4 --addr --rdns
{
my $cmd = Test::Command->new(cmd => "fping -4 --addr --rdns www.apple.com");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr{^\S+\.akamaitechnologies\.com \(\d+\.\d+\.\d+\.\d+\) is alive\n$});
$cmd->stderr_is_eq("");
}

# fping -4 --addr --name
{
my $cmd = Test::Command->new(cmd => "fping -4 --addr --name www.google.com");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr{^www\.google\.com \(\d+\.\d+\.\d+\.\d+\) is alive\n$});
$cmd->stderr_is_eq("");
}

# fping -A -n (IPv6)
SKIP: {
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 3;
    }
    my $cmd = Test::Command->new(cmd => "fping -6 -n -A dns.google");
    $cmd->exit_is_num(0);
    $cmd->stdout_like(qr{^dns.google \(2001:4860:4860::88(44|88)\) is alive\n$});
    $cmd->stderr_is_eq("");
}

# fping -m
SKIP: {
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 3;
    }
    my $cmd = Test::Command->new(cmd => "fping -A -m dns.google");
    $cmd->exit_is_num(0);
    $cmd->stdout_like(qr{^.* is alive\n.* is alive\n.* is alive\n.* is alive\n});
    $cmd->stderr_is_eq("");
}

# fping -m -A
{
my $cmd = Test::Command->new(cmd => "fping -4 -A -m www.cloudflare.com");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr{\d+\.\d+\.\d+\.\d+ is alive\n\d+\.\d+\.\d+\.\d+ is alive\n});
$cmd->stderr_is_eq("");
}

# fping -n
{
my $cmd = Test::Command->new(cmd => "fping -n 8.8.8.8");
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("dns.google is alive\n");
$cmd->stderr_is_eq("");
}

# fping -M
SKIP: {
    if($^O eq 'darwin') {
        skip '-M option not supported on macOS', 3;
    }
    my $cmd = Test::Command->new(cmd => "fping -r 0 -b 10000 -M 8.8.8.8");
    $cmd->exit_is_num(1);
    $cmd->stdout_is_eq("8.8.8.8 is unreachable\n");
    $cmd->stderr_is_eq("8.8.8.8: error while sending ping: Message too long\n");
}
