#!/usr/bin/perl -w

use Test::Command tests => 9;
my $fping_bin  = '/opt/fping/sbin/fping';
my $fping6_bin = '/opt/fping/sbin/fping6';
#my $fping_bin  = '/home/dws/checkouts/fping/src/fping';
#my $fping6_bin = '/home/dws/checkouts/fping/src/fping6';

# ping 127.0.0.1
my $cmd1 = Test::Command->new(cmd => "sudo $fping_bin 127.0.0.1");
$cmd1->exit_is_num(0);
$cmd1->stdout_is_eq("127.0.0.1 is alive\n");
$cmd1->stderr_is_eq("");

# ping ::1
my $cmd2 = Test::Command->new(cmd => "sudo $fping6_bin ::1");
$cmd2->exit_is_num(0);
$cmd2->stdout_is_eq("::1 is alive\n");
$cmd2->stderr_is_eq("");

# ping 3 times 127.0.0.1
my $cmd3 = Test::Command->new(cmd => "sudo $fping_bin -p 100 -C3 127.0.0.1");
$cmd3->exit_is_num(0);
$cmd3->stdout_like(qr{127\.0\.0\.1 : \[0\], 84 bytes, 0\.\d+ ms \(0\.\d+ avg, 0% loss\)
127\.0\.0\.1 : \[1\], 84 bytes, 0\.\d+ ms \(0.\d+ avg, 0% loss\)
127\.0\.0\.1 : \[2\], 84 bytes, 0\.\d+ ms \(0.\d+ avg, 0% loss\)
});
$cmd3->stderr_like(qr{127\.0\.0\.1 : 0\.\d+ 0\.\d+ 0\.\d+\n});
