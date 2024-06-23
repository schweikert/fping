#!/usr/bin/perl -w

use Test::Command tests => 15;
use Test::More;

#  -i n       interval between sending ping packets (in millisec) (default 25)
#  -l         loop sending pings forever
#  -k         set fwmark on ping packets
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
$cmd->stdout_like(qr{127\.0\.0\.1 : \[0\], 64 bytes, \d\.\d+ ms \(\d\.\d+ avg, 0% loss\)
127\.0\.0\.1 : \[1\], 64 bytes, \d\.\d+ ms \(\d\.\d+ avg, 0% loss\)
});
}

# fping -k
SKIP: {
if($^O ne 'linux') {
    skip '-k option is only supported on Linux', 3;
}
my $cmd = Test::Command->new(cmd => 'sudo env "PATH=$PATH" fping -k 256 127.0.0.1');
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("127.0.0.1 is alive\n");
$cmd->stderr_is_eq("");
}

# fping -l with SIGQUIT
{
my $cmd = Test::Command->new(cmd => '(sleep 2; pkill -QUIT fping; sleep 2; pkill fping)& fping -p 900 -l 127.0.0.1');
$cmd->stdout_like(qr{127\.0\.0\.1 : \[0\], 64 bytes, \d\.\d+ ms \(\d\.\d+ avg, 0% loss\)
127\.0\.0\.1 : \[1\], 64 bytes, \d\.\d+ ms \(\d\.\d+ avg, 0% loss\)
127\.0\.0\.1 : \[2\], 64 bytes, \d\.\d+ ms \(\d\.\d+ avg, 0% loss\)
127\.0\.0\.1 : \[3\], 64 bytes, \d\.\d+ ms \(\d\.\d+ avg, 0% loss\)
127\.0\.0\.1 : \[4\], 64 bytes, \d\.\d+ ms \(\d\.\d+ avg, 0% loss\)
});
$cmd->stderr_like(qr{\[\d+:\d+:\d+\]
127\.0\.0\.1 : xmt/rcv/%loss = \d+/\d+/\d+%, min/avg/max = \d+\.\d+/\d+\.\d+/\d+\.\d+
});
}

# fping -l -Q
{
my $cmd = Test::Command->new(cmd => '(sleep 2; pkill fping)& fping -p 770 -l -Q 1 127.0.0.1');
$cmd->stdout_is_eq("");
$cmd->stderr_like(qr{\[\d\d:\d\d:\d\d\]
127\.0\.0\.1 : xmt/rcv/%loss = \d/\d/\d%, min/avg/max = \d\.\d+/\d\.\d+/\d\.\d+
\[\d\d:\d\d:\d\d\]
127\.0\.0\.1 : xmt/rcv/%loss = \d/\d/\d%, min/avg/max = \d\.\d+/\d\.\d+/\d\.\d+
});
}

# fping -l -t
{
my $cmd = Test::Command->new(cmd => '(sleep 2; pkill fping)& fping -p 850 -t 1500 -l 127.0.0.1');
$cmd->stdout_like(qr{127\.0\.0\.1 : \[0\], 64 bytes, \d\.\d+ ms \(\d\.\d+ avg, 0% loss\)
127\.0\.0\.1 : \[1\], 64 bytes, \d\.\d+ ms \(\d\.\d+ avg, 0% loss\)
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
