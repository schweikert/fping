#!/usr/bin/perl -w

use Test::Command tests => 7;
use Test::More;

#  -i n       interval between sending ping packets (in millisec) (default 25)
#  -l         loop sending pings forever
#  -m         ping multiple interfaces on target host
#  -M         don't fragment

# fping -i n
{
my $cmd = Test::Command->new(cmd => "fping -i 100 127.0.0.1 127.0.0.2");
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("127.0.0.1 is alive\n127.0.0.2 is alive\n");
$cmd->stderr_is_eq("");
}

# fping -l
{
my $cmd = Test::Command->new(cmd => '(sleep 2; pkill fping)& fping -p 900 -l 127.0.0.1');
$cmd->stdout_like(qr{127\.0\.0\.1 : \[0\], 84 bytes, 0\.\d+ ms \(0.\d+ avg, 0% loss\)
127\.0\.0\.1 : \[1\], 84 bytes, 0\.\d+ ms \(0\.\d+ avg, 0% loss\)
});
}

# fping -M
SKIP: {
    if($^O eq 'darwin') {
        skip '-M option not supported on macOS', 3;
    }
    my $cmd = Test::Command->new(cmd => "fping -M 127.0.0.1");
    $cmd->exit_is_num(0);
    $cmd->stdout_is_eq("127.0.0.1 is alive\n");
    $cmd->stderr_is_eq("");
}

# fping -m -> test-14-internet-hosts
