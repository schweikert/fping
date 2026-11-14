#!/usr/bin/perl -w

use Test::Command tests => 18;

my $I_HELP = "   -I, --iface=IFACE  bind to a particular interface\n";
$I_HELP = '' if $^O eq 'darwin';

# fping -h (special pre-parse code path)
my $cmd1 = Test::Command->new(cmd => "fping -h");
$cmd1->exit_is_num(0);
$cmd1->stdout_like(qr{Usage: fping \[options\] \[targets\.\.\.\]

Probing options:
.*
   -v, --version      show version
}s);
$cmd1->stderr_is_eq("");

# fping -4 -h (normal option parsing code path)
my $cmd4 = Test::Command->new(cmd => "fping -4 -h");
$cmd4->exit_is_num(0);
$cmd4->stdout_like(qr{Usage: fping \[options\] \[targets\.\.\.\]

Probing options:
.*
   -v, --version      show version
}s);
$cmd4->stderr_is_eq("");

# fping -v
my $cmd2 = Test::Command->new(cmd => "fping -v");
$cmd2->exit_is_num(0);
$cmd2->stdout_like(qr{fping: Version \S+});
$cmd2->stderr_is_eq("");

# fping with unknown option
my $cmd3 = Test::Command->new(cmd => "fping -Z");
$cmd3->exit_is_num(1);
$cmd3->stdout_is_eq("");
$cmd3->stderr_like(qr{^fping: (illegal|invalid) option -- '?Z'?\nsee 'fping -h' for usage information\n$});

# fping with unknown long option
my $cmd5 = Test::Command->new(cmd => "fping --unknown-long-option");
$cmd5->exit_is_num(1);
$cmd5->stdout_is_eq("");
$cmd5->stderr_like(qr{^fping: (illegal|invalid) option -- '?unknown-long-option'?\nsee 'fping -h' for usage information\n$});

# fping -h with overridden timing options still reports compile-time defaults
my $cmd6 = Test::Command->new(cmd => "fping -i 50 -t 1234 -p 2000 --seqmap-timeout=5000 -h");
$cmd6->exit_is_num(0);
$cmd6->stdout_like(qr{-i, --interval=MSEC .*?\(default: 10 ms\).*-p, --period=MSEC .*?\(in loop and count modes, default: 1000 ms\).*--seqmap-timeout=MSEC .*?\(default: 10000 ms\).*-t, --timeout=MSEC .*?\(default: 500 ms,}s);
$cmd6->stderr_is_eq("");
