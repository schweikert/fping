#!/usr/bin/perl -w

use Test::Command tests => 6;

# fping
{
my $cmd = Test::Command->new(cmd => "fping nosuchname.example.com");
$cmd->exit_is_num(1);
$cmd->stdout_is_eq("");
$cmd->stderr_is_eq("nosuchname.example.com: Name or service not known\n");
}

# fping6
{
my $cmd = Test::Command->new(cmd => "fping6 nosuchname.example.com");
$cmd->exit_is_num(1);
$cmd->stdout_is_eq("");
$cmd->stderr_is_eq("nosuchname.example.com: Name or service not known\n");
}
