#!/usr/bin/perl -w

use Test::Command tests => 24;
use File::Temp;

#  -f file    read list of targets from a file ( - means stdin) (only if no -g specified)
#  -g         generate target list (only if no -f specified)
#               (specify the start and end IP in the target list, or supply a IP netmask)
#               (ex. ../src/fping -g 192.168.1.0 192.168.1.255 or ../src/fping -g 192.168.1.0/24)
#  -H n       Set the IP TTL value (Time To Live hops)

my $tmpfile = File::Temp->new();
print $tmpfile "127.0.0.1\n127.0.0.2\n";
close($tmpfile);

# fping without option (-> equivalent to 'fping -f -')
{
my $cmd = Test::Command->new(cmd => "cat ".$tmpfile->filename." | fping");
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("127.0.0.1 is alive\n127.0.0.2 is alive\n");
$cmd->stderr_is_eq("");
}

# fping -f -
{
my $cmd = Test::Command->new(cmd => "cat ".$tmpfile->filename." | fping -f -");
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("127.0.0.1 is alive\n127.0.0.2 is alive\n");
$cmd->stderr_is_eq("");
}

# fping -f file
{
my $cmd = Test::Command->new(cmd => "fping -f ".$tmpfile->filename);
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("127.0.0.1 is alive\n127.0.0.2 is alive\n");
$cmd->stderr_is_eq("");
}

# fping -g (range)
{
my $cmd = Test::Command->new(cmd => "fping -g 127.0.0.1 127.0.0.5");
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("127.0.0.1 is alive\n127.0.0.2 is alive\n127.0.0.3 is alive\n127.0.0.4 is alive\n127.0.0.5 is alive\n");
$cmd->stderr_is_eq("");
}

# fping -g (cidr)
{
my $cmd = Test::Command->new(cmd => "fping -g 127.0.0.1/30");
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("127.0.0.1 is alive\n127.0.0.2 is alive\n");
$cmd->stderr_is_eq("");
}

# fping -g (cidr - long prefixes)
{
my $cmd = Test::Command->new(cmd => "fping -g 127.0.0.2/31");
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("127.0.0.2 is alive\n127.0.0.3 is alive\n");
$cmd->stderr_is_eq("");
}

# fping -g (cidr - too long prefixes)
{
my $cmd = Test::Command->new(cmd => "fping -g 127.0.0.2/33");
$cmd->exit_is_num(1);
$cmd->stdout_is_eq("");
$cmd->stderr_is_eq("fping: netmask must be between 1 and 32 (is: 33)\n");
}

# fping -H
{
my $cmd = Test::Command->new(cmd => "fping -H 1 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("127.0.0.1 is alive\n");
$cmd->stderr_is_eq("");
}
