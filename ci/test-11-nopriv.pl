#!/usr/bin/perl -w

use Test::Command tests => 6;

# run without privileges
my $fping_bin = `which fping`; chomp $fping_bin;
my $fping6_bin = `which fping6`; chomp $fping6_bin;
system("cp $fping_bin /tmp/fping.copy; chmod +x /tmp/fping.copy");
system("cp $fping6_bin /tmp/fping6.copy; chmod +x /tmp/fping6.copy");

# fping
{
my $cmd = Test::Command->new(cmd => "/tmp/fping.copy 127.0.0.1");
$cmd->exit_is_num(4);
$cmd->stdout_is_eq("");
$cmd->stderr_is_eq("(null): can't create socket (must run as root?) : Permission denied\n");
}

# fping6
{
my $cmd = Test::Command->new(cmd => "/tmp/fping6.copy ::1");
$cmd->exit_is_num(4);
$cmd->stdout_is_eq("");
$cmd->stderr_is_eq("(null): can't create raw socket (must run as root?) : Protocol not supported\n");
}
