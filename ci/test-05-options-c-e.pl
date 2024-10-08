#!/usr/bin/perl -w

use Test::Command tests => 78;
use Test::More;

#  -c n           count of pings to send to each target (default 1)
#  -C n           same as -c, report results in verbose format
#  --check-source discard replies not from target address
#  -d             reverse name lookup
#  -D             print timestamp before each output line
#  -e             show elapsed time on return packets

# fping -c n
{
my $cmd = Test::Command->new(cmd => "fping -4 -c 2 -p 100 localhost 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr{localhost : \[0\], 64 bytes, \d\.\d+ ms \(\d\.\d+ avg, 0% loss\)
127\.0\.0\.1 : \[0\], 64 bytes, \d\.\d+ ms \(\d\.\d+ avg, 0% loss\)
localhost : \[1\], 64 bytes, \d\.\d+ ms \(\d\.\d+ avg, 0% loss\)
127\.0\.0\.1 : \[1\], 64 bytes, \d\.\d+ ms \(\d\.\d+ avg, 0% loss\)
});

$cmd->stderr_like(qr{localhost : xmt/rcv/%loss = 2/2/0%, min/avg/max = \d\.\d+/\d\.\d+/\d\.\d+
127\.0\.0\.1 : xmt/rcv/%loss = 2/2/0%, min/avg/max = \d\.\d+/\d\.\d+/\d\.\d+
});
}

# fping -c n -q
{
my $cmd = Test::Command->new(cmd => "fping -q -c 2 -p 100 localhost 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("");
$cmd->stderr_like(qr{localhost : xmt/rcv/%loss = 2/2/0%, min/avg/max = \d\.\d+/\d\.\d+/\d\.\d+
127\.0\.0\.1 : xmt/rcv/%loss = 2/2/0%, min/avg/max = \d\.\d+/\d\.\d+/\d\.\d+
});
}

# fping -c n -a (-a is ignored)
{
my $cmd = Test::Command->new(cmd => "fping -a -c 2 -p 100 localhost 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr{localhost : \[0\], 64 bytes, \d\.\d+ ms \(\d\.\d+ avg, 0% loss\)
127\.0\.0\.1 : \[0\], 64 bytes, \d\.\d+ ms \(\d\.\d+ avg, 0% loss\)
localhost : \[1\], 64 bytes, \d\.\d+ ms \(\d\.\d+ avg, 0% loss\)
127\.0\.0\.1 : \[1\], 64 bytes, \d\.\d+ ms \(\d\.\d+ avg, 0% loss\)
});

$cmd->stderr_like(qr{localhost : xmt/rcv/%loss = 2/2/0%, min/avg/max = \d\.\d+/\d\.\d+/\d\.\d+
127\.0\.0\.1 : xmt/rcv/%loss = 2/2/0%, min/avg/max = \d\.\d+/\d\.\d+/\d\.\d+
});
}

# fping -c n -u (-u is ignored)
{
my $cmd = Test::Command->new(cmd => "fping -u -c 2 -p 100 localhost 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr{localhost : \[0\], 64 bytes, \d\.\d+ ms \(\d\.\d+ avg, 0% loss\)
127\.0\.0\.1 : \[0\], 64 bytes, \d\.\d+ ms \(\d\.\d+ avg, 0% loss\)
localhost : \[1\], 64 bytes, \d\.\d+ ms \(\d\.\d+ avg, 0% loss\)
127\.0\.0\.1 : \[1\], 64 bytes, \d\.\d+ ms \(\d\.\d+ avg, 0% loss\)
});

$cmd->stderr_like(qr{localhost : xmt/rcv/%loss = 2/2/0%, min/avg/max = \d\.\d+/\d\.\d+/\d\.\d+
127\.0\.0\.1 : xmt/rcv/%loss = 2/2/0%, min/avg/max = \d\.\d+/\d\.\d+/\d\.\d+
});
}

# fping -c n ff02::1
SKIP: {
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 3;
    }
    my $cmd = Test::Command->new(cmd => "fping -c 1 ff02::1");
    $cmd->exit_is_num(0);
    $cmd->stdout_like(qr{ff02::1 : \[0\], 64 bytes, \d\.\d+ ms \(\d\.\d+ avg, 0% loss\)\n});
    $cmd->stderr_like(qr{ \[<- .*\]
ff02::1 : xmt/rcv/%loss = 1/1/0%, min/avg/max = \d\.\d+/\d\.\d+/\d\.\d+\n});
}

# fping --icmp-timestamp -c n 127.0.0.1
SKIP: {
if($^O eq 'darwin') {
    skip 'On macOS, this test is unreliable', 3;
}
my $cmd = Test::Command->new(cmd => "fping -4 --icmp-timestamp -c 2 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr{127\.0\.0\.1 : \[0\], 20 bytes, \d\.\d+ ms \(\d\.\d+ avg, 0% loss\)
ICMP timestamp: Originate=\d+ Receive=\d+ Transmit=\d+
ICMP timestamp RTT tsrtt=\d+
127\.0\.0\.1 : \[1\], 20 bytes, \d\.\d+ ms \(\d\.\d+ avg, 0% loss\)
ICMP timestamp: Originate=\d+ Receive=\d+ Transmit=\d+
ICMP timestamp RTT tsrtt=\d+
});

$cmd->stderr_like(qr{127\.0\.0\.1 : xmt/rcv/%loss = 2/2/0%, min/avg/max = \d\.\d+/\d\.\d+/\d\.\d+
});
}

# fping -C n
{
my $cmd = Test::Command->new(cmd => "fping -4 -C 2 -p 100 localhost 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr{localhost : \[0\], 64 bytes, \d\.\d+ ms \(\d\.\d+ avg, 0% loss\)
127\.0\.0\.1 : \[0\], 64 bytes, \d\.\d+ ms \(\d\.\d+ avg, 0% loss\)
localhost : \[1\], 64 bytes, \d\.\d+ ms \(\d\.\d+ avg, 0% loss\)
127\.0\.0\.1 : \[1\], 64 bytes, \d\.\d+ ms \(\d\.\d+ avg, 0% loss\)
});

$cmd->stderr_like(qr{localhost : \d\.\d+ \d\.\d+
127\.0\.0\.1 : \d\.\d+ \d\.\d+
});
}

# fping -C n -q
{
my $cmd = Test::Command->new(cmd => "fping -C 5 -q -p 100 localhost");
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("");
$cmd->stderr_like(qr{localhost :( \d\.\d+){5}
});
}

# fping -C n -a (-a is ignored)
{
my $cmd = Test::Command->new(cmd => "fping -a -C 2 -p 100 localhost 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr{localhost : \[0\], 64 bytes, \d\.\d+ ms \(\d\.\d+ avg, 0% loss\)
127\.0\.0\.1 : \[0\], 64 bytes, \d\.\d+ ms \(\d\.\d+ avg, 0% loss\)
localhost : \[1\], 64 bytes, \d\.\d+ ms \(\d\.\d+ avg, 0% loss\)
127\.0\.0\.1 : \[1\], 64 bytes, \d\.\d+ ms \(\d\.\d+ avg, 0% loss\)
});

$cmd->stderr_like(qr{localhost : \d\.\d+ \d\.\d+
127\.0\.0\.1 : \d\.\d+ \d\.\d+
});
}

# fping -C n -u (-u is ignored)
{
my $cmd = Test::Command->new(cmd => "fping -u -C 2 -p 100 localhost 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr{localhost : \[0\], 64 bytes, \d\.\d+ ms \(\d\.\d+ avg, 0% loss\)
127\.0\.0\.1 : \[0\], 64 bytes, \d\.\d+ ms \(\d\.\d+ avg, 0% loss\)
localhost : \[1\], 64 bytes, \d\.\d+ ms \(\d\.\d+ avg, 0% loss\)
127\.0\.0\.1 : \[1\], 64 bytes, \d\.\d+ ms \(\d\.\d+ avg, 0% loss\)
});

$cmd->stderr_like(qr{localhost : \d\.\d+ \d\.\d+
127\.0\.0\.1 : \d\.\d+ \d\.\d+
});
}

# fping -C n -i -q
{
my $cmd = Test::Command->new(cmd => "fping --quiet --interval=1 --vcount=20 --period=50 127.0.0.1 127.0.0.2");
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("");
$cmd->stderr_like(qr{127\.0\.0\.1 :( \d\.\d+){20}
127\.0\.0\.2 :( \d\.\d+){20}
});
}

# fping -C n ff02::1
SKIP: {
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 3;
    }
    my $cmd = Test::Command->new(cmd => "fping -C 1 ff02::1");
    $cmd->exit_is_num(0);
    $cmd->stdout_like(qr{ff02::1 : \[0\], 64 bytes, \d\.\d+ ms \(\d\.\d+ avg, 0% loss\)\n});
    $cmd->stderr_like(qr{ \[<- .*\]\nff02::1 : \d\.\d+\n});
}

# fping --check-source
{
my $cmd = Test::Command->new(cmd => "fping --check-source 127.0.0.1 127.0.0.2");
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("127.0.0.1 is alive\n127.0.0.2 is alive\n");
$cmd->stderr_is_eq("");
}

# fping --check-source (to IPv6 multicast address -> accept no reply)
SKIP: {
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 3;
    }
    my $cmd = Test::Command->new(cmd => "fping --check-source ff02::1");
    $cmd->exit_is_num(1);
    $cmd->stdout_is_eq("ff02::1 is unreachable\n");
    $cmd->stderr_is_eq("");
}

# fping -c N --check-source
SKIP: {
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 3;
    }
    my $cmd = Test::Command->new(cmd => "fping -c1 --check-source 127.0.0.1 ff02::1");
    $cmd->exit_is_num(1);
    $cmd->stdout_like(qr{127\.0\.0\.1 : \[0\], 64 bytes, \d\.\d+ ms \(\d\.\d+ avg, 0% loss\)
ff02::1   : \[0\], timed out \(NaN avg, 100% loss\)
});
    $cmd->stderr_like(qr{
127\.0\.0\.1 : xmt/rcv/%loss = 1/1/0%, min/avg/max = \d\.\d+/\d\.\d+/\d\.\d+
ff02::1   : xmt/rcv/%loss = 1/0/100%
});
}

# fping -C N --check-source
SKIP: {
    if($ENV{SKIP_IPV6}) {
        skip 'Skip IPv6 tests', 3;
    }
    my $cmd = Test::Command->new(cmd => "fping -C1 --check-source 127.0.0.1 ff02::1");
    $cmd->exit_is_num(1);
    $cmd->stdout_like(qr{127\.0\.0\.1 : \[0\], 64 bytes, \d\.\d+ ms \(\d\.\d+ avg, 0% loss\)
ff02::1   : \[0\], timed out \(NaN avg, 100% loss\)
});
    $cmd->stderr_like(qr{
127\.0\.0\.1 : \d\.\d+
ff02::1   : -
});
}

# fping -d
{
my $cmd = Test::Command->new(cmd => "fping -d 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("localhost is alive\n");
$cmd->stderr_is_eq("");
}

# fping -D
{
my $cmd = Test::Command->new(cmd => "fping -D -c 2 -p 100 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr{\[\d+\.\d+\] 127\.0\.0\.1 : \[0\], 64 bytes, \d\.\d+ ms \(\d\.\d+ avg, 0% loss\)
\[\d+\.\d+\] 127\.0\.0\.1 : \[1\], 64 bytes, \d\.\d+ ms \(\d\.\d+ avg, 0% loss\)
});

$cmd->stderr_like(qr{127\.0\.0\.1 : xmt/rcv/%loss = 2/2/0%, min/avg/max = \d\.\d+/\d\.\d+/\d\.\d+
});
}

# fping -D (timestamp not before 2001-09-09)
{
my $cmd = Test::Command->new(cmd => "fping -D -c 2 -p 100 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr{\[[1-9]\d{9,}\.\d+\] 127\.0\.0\.1 : \[0\], 64 bytes, \d\.\d+ ms \(\d\.\d+ avg, 0% loss\)
\[[1-9]\d{9,}\.\d+\] 127\.0\.0\.1 : \[1\], 64 bytes, \d\.\d+ ms \(\d\.\d+ avg, 0% loss\)
});
$cmd->stderr_like(qr{127\.0\.0\.1 : xmt/rcv/%loss = 2/2/0%, min/avg/max = \d\.\d+/\d\.\d+/\d\.\d+
});
}

# fping -D --timestamp-format=ctime
{
my $cmd = Test::Command->new(cmd => "fping -D --timestamp-format=ctime -c 2 -p 100 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr{\[\w+\s\w+\s+\d+\s[\d+:]+\s\d+\] 127\.0\.0\.1 : \[0\], 64 bytes, \d\.\d+ ms \(\d\.\d+ avg, 0% loss\)
\[\w+\s\w+\s+\d+\s[\d+:]+\s\d+\] 127\.0\.0\.1 : \[1\], 64 bytes, \d\.\d+ ms \(\d\.\d+ avg, 0% loss\)
});

$cmd->stderr_like(qr{127\.0\.0\.1 : xmt/rcv/%loss = 2/2/0%, min/avg/max = \d\.\d+/\d\.\d+/\d\.\d+
});
}

# fping -D --timestamp-format=iso
{
my $cmd = Test::Command->new(cmd => "fping -D --timestamp-format=iso -c 2 -p 100 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr{\[[\d+-]+T[\d+:]+\+\d+\] 127\.0\.0\.1 : \[0\], 64 bytes, \d\.\d+ ms \(\d\.\d+ avg, 0% loss\)
\[[\d+-]+T[\d+:]+\+\d+\] 127\.0\.0\.1 : \[1\], 64 bytes, \d\.\d+ ms \(\d\.\d+ avg, 0% loss\)
});

$cmd->stderr_like(qr{127\.0\.0\.1 : xmt/rcv/%loss = 2/2/0%, min/avg/max = \d\.\d+/\d\.\d+/\d\.\d+
});
}

# fping -D --timestamp-format=rfc3339
{
my $cmd = Test::Command->new(cmd => "fping -D --timestamp-format=rfc3339 -c 2 -p 100 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr{\[[\d+-]+\s[\d+:]+\] 127\.0\.0\.1 : \[0\], 64 bytes, \d\.\d+ ms \(\d\.\d+ avg, 0% loss\)
\[[\d+-]+\s[\d+:]+\] 127\.0\.0\.1 : \[1\], 64 bytes, \d\.\d+ ms \(\d\.\d+ avg, 0% loss\)
});

$cmd->stderr_like(qr{127\.0\.0\.1 : xmt/rcv/%loss = 2/2/0%, min/avg/max = \d\.\d+/\d\.\d+/\d\.\d+
});
}

# fping -D --timestamp-format
{
my $cmd = Test::Command->new(cmd => "fping -D --timestamp-format -c 2 -p 100 127.0.0.1");
$cmd->exit_is_num(1);
$cmd->stdout_is_eq("");
$cmd->stderr_like(qr{Usage:});
}

# fping -D --timestamp-format="%Y-%m-%d %H:%M:%S"
{
my $cmd = Test::Command->new(cmd => "fping -D --timestamp-format=\"%Y-%m-%d %H:%M:%S\" -c 2 -p 100 127.0.0.1");
$cmd->exit_is_num(1);
$cmd->stdout_is_eq("");
$cmd->stderr_like(qr{Usage:});
}

# fping -e
{
my $cmd = Test::Command->new(cmd => "fping -e 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr{127\.0\.0\.1 is alive \(\d\.\d+ ms\)
});
$cmd->stderr_is_eq("");
}

# fping -e -a
{
my $cmd = Test::Command->new(cmd => "fping -e -a 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr{127\.0\.0\.1 \(\d\.\d+ ms\)
});
$cmd->stderr_is_eq("");
}
