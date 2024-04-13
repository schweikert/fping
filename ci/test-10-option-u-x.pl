#!/usr/bin/perl -w

use Test::Command tests => 15;

#  -u         show targets that are unreachable
#  -v         show version
#  -x         shows if >=N hosts are reachable or not
#  -X         exits true immediately when N hosts are found

# fping -u
{
my $cmd = Test::Command->new(cmd => "fping -r0 -u 8.8.0.0 127.0.0.1");
$cmd->exit_is_num(1);
$cmd->stdout_is_eq("8.8.0.0\n");
$cmd->stderr_is_eq("");
}

# fping -v
{
my $cmd = Test::Command->new(cmd => "fping -v");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr{ping: Version [45]\.\d+(-rc\d+)?});
$cmd->stderr_is_eq("");
}

# fping -x
{
my $cmd = Test::Command->new(cmd => "fping -x 1 8.8.0.0 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("Enough hosts reachable (required: 1, reachable: 1)\n");
$cmd->stderr_is_eq("");
}

# fping -x
{
my $cmd = Test::Command->new(cmd => "fping -x 2 8.8.0.0 127.0.0.1");
$cmd->exit_is_num(1);
$cmd->stdout_is_eq("Not enough hosts reachable (required: 2, reachable: 1)\n");
$cmd->stderr_is_eq("");
}

# fping -X
{
my $cmd = Test::Command->new(cmd => "fping -X 1 --generate 127.0.0.0/29");
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("Enough hosts reachable (required: 1, reachable: 1)\n");
$cmd->stderr_is_eq("");
}
