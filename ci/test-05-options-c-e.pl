#!/usr/bin/perl -w

use Test::Command tests => 12;

#  -c n       count of pings to send to each target (default 1)
#  -C n       same as -c, report results in verbose format
#  -D         print timestamp before each output line
#  -e         show elapsed time on return packets

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

# fping -e
{
my $cmd = Test::Command->new(cmd => "fping -e 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr{127\.0\.0\.1 is alive \(\d\.\d+ ms\)
});

$cmd->stderr_is_eq("");
}
