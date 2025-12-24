#!/usr/bin/perl -w

use Test::Command tests => 21;
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

# ping ff02::1
SKIP: {
    #system("/sbin/ifconfig >&2");
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 3;
    }
    my $cmd = Test::Command->new(cmd => "fping ff02::1");
    $cmd->exit_is_num(0);
    $cmd->stdout_is_eq("ff02::1 is alive\n");
    $cmd->stderr_like(qr{ \[<- .*\]});
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

# invalid target name
{
    my $cmd = Test::Command->new(cmd => "fping host.name.invalid");
    $cmd->exit_is_num(2);
    $cmd->stdout_is_eq("");
    $cmd->stderr_like(qr{host\.name\.invalid: .+\n});
}

# Mixed arguments to exercise optparse permutation
{
    my $cmd = Test::Command->new(cmd => "fping 127.0.0.1 -c 1");
    $cmd->exit_is_num(0);
    $cmd->stdout_like(qr{127\.0\.0\.1 : \[0\], 64 bytes});
    $cmd->stderr_like(qr{\s*127\.0\.0\.1\s+:\sxmt/rcv/%loss = 1/1/0%, min/avg/max});
}

# fping6 symlink compatibility
SKIP: {
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 3;
    }

    symlink "fping", "src/fping6" or die "Failed to create symlink: $!";

    my $cmd = Test::Command->new(cmd => "./src/fping6 ::1 -c 1");
    $cmd->exit_is_num(0);
    $cmd->stdout_like(qr{::1 : \[0\], 64 bytes});
    $cmd->stderr_like(qr{^\s*::1 : xmt/rcv/%loss = 1/1/0%, min/avg/max = .*$});

    unlink "src/fping6";
}
