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
    if(system("/sbin/ifconfig | grep inet6") != 0) {
        skip 'No IPv6 on this host', 3;
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
    $cmd->stdout_like(qr{127\.0\.0\.1 : \[0\], 84 bytes, 0\.\d+ ms \(0\.\d+ avg, 0% loss\)
127\.0\.0\.1 : \[1\], 84 bytes, 0\.\d+ ms \(0.\d+ avg, 0% loss\)
127\.0\.0\.1 : \[2\], 84 bytes, 0\.\d+ ms \(0.\d+ avg, 0% loss\)
});
    $cmd->stderr_like(qr{127\.0\.0\.1 : 0\.\d+ 0\.\d+ 0\.\d+\n});
}
