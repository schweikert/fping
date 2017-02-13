#!/usr/bin/perl -w

use Test::Command tests => 9;

my $I_HELP = "   -I, --iface=IFACE  bind to a particular interface\n";
$I_HELP = '' if $^O eq 'darwin';

# fping -h
my $cmd1 = Test::Command->new(cmd => "fping -h");
$cmd1->exit_is_num(0);
$cmd1->stdout_like(qr{Usage: fping \[options\] \[targets\.\.\.\]

Probing options:
.*
   -v, --version      show version
}s);
$cmd1->stderr_is_eq("");

# fping -v
my $cmd2 = Test::Command->new(cmd => "fping -v");
$cmd2->exit_is_num(0);
$cmd2->stdout_like(qr{fping: Version \S+
fping: comments to david\@schweikert\.ch\n});
$cmd2->stderr_is_eq("");

# fping with unknown option
my $cmd3 = Test::Command->new(cmd => "fping -Z");
$cmd3->exit_is_num(1);
$cmd3->stdout_is_eq("");
$cmd3->stderr_like(qr{^fping: (illegal|invalid) option -- '?Z'?\nsee 'fping -h' for usage information\n$});
