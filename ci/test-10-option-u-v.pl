#!/usr/bin/perl -w

use Test::Command tests => 6;

#  -u         show targets that are unreachable
#  -v         show version

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
$cmd->stdout_like(qr{ping: Version 4\.\d+(-rc\d+)?
fping: comments to david\@schweikert\.ch
});
$cmd->stderr_is_eq("");
}
