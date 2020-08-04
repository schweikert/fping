#!/usr/bin/perl -w

use Test::Command tests => 9;
use Test::More;

# ping 127.0.0.1
{
    my $cmd = Test::Command->new(cmd => "fping 127.0.0.1");
    $cmd->exit_is_num(0);
    $cmd->stdout_is_eq("127.0.0.1 is alive\n");
    $cmd->stderr_is_eq("");
}

# ping ::1
SKIP: {
    #system("/sbin/ifconfig >&2");
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 3;
    }
    my $cmd = Test::Command->new(cmd => "fping ::1");
    $cmd->exit_is_num(0);
    $cmd->stdout_is_eq("::1 is alive\n");
    $cmd->stderr_is_eq("");
}

# ping 3 times 127.0.0.1
{
    my $cmd = Test::Command->new(cmd => "fping -p 100 -C3 127.0.0.1");
    $cmd->exit_is_num(0);
    $cmd->stdout_like(qr{127\.0\.0\.1 : \[0\], 64 bytes, \d\.\d+ ms \(\d\.\d+ avg, 0% loss\)
127\.0\.0\.1 : \[1\], 64 bytes, \d\.\d+ ms \(\d\.\d+ avg, 0% loss\)
127\.0\.0\.1 : \[2\], 64 bytes, \d\.\d+ ms \(\d\.\d+ avg, 0% loss\)
});
    $cmd->stderr_like(qr{127\.0\.0\.1 : \d\.\d+ \d\.\d+ \d\.\d+\n});
}
