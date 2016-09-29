#!/usr/bin/perl -w

use Test::Command tests => 9;

my $I_HELP = "   -I if      bind to a particular interface\n";
$I_HELP = '' if $^O eq 'darwin';

# fping -h
my $cmd1 = Test::Command->new(cmd => "fping -h");
$cmd1->exit_is_num(0);
$cmd1->stdout_is_eq(<<END);

Usage: fping [options] [targets...]
   -a         show targets that are alive
   -A         show targets by address
   -b n       amount of ping data to send, in bytes (default 56)
   -B f       set exponential backoff factor to f
   -c n       count of pings to send to each target (default 1)
   -C n       same as -c, report results in verbose format
   -D         print timestamp before each output line
   -e         show elapsed time on return packets
   -f file    read list of targets from a file ( - means stdin) (only if no -g specified)
   -g         generate target list (only if no -f specified)
                (specify the start and end IP in the target list, or supply a IP netmask)
                (ex. fping -g 192.168.1.0 192.168.1.255 or fping -g 192.168.1.0/24)
   -H n       Set the IP TTL value (Time To Live hops)
   -i n       interval between sending ping packets (in millisec) (default 25)
${I_HELP}   -l         loop sending pings forever
   -m         ping multiple interfaces on target host
   -M         set the Don't Fragment flag
   -n         show targets by name (-d is equivalent)
   -o         show the accumulated outage time (lost packets * packet interval)
   -O n       set the type of service (tos) flag on the ICMP packets
   -p n       interval between ping packets to one target (in millisec)
                (in looping and counting modes, default 1000)
   -q         quiet (don't show per-target/per-ping results)
   -Q n       same as -q, but show summary every n seconds
   -r n       number of retries (default 3)
   -R         random packet data (to foil link data compression)
   -s         print final stats
   -S addr    set source address
   -t n       individual target initial timeout (in millisec) (default 500)
   -T n       ignored (for compatibility with fping 2.4)
   -u         show targets that are unreachable
   -v         show version
   targets    list of targets to check (if no -f specified)

END
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
if($^O eq 'darwin') {
    $cmd3->stderr_is_eq("fping: illegal option -- Z\nsee 'fping -h' for usage information\n");
}
else {
    $cmd3->stderr_is_eq("fping: invalid option -- 'Z'\nsee 'fping -h' for usage information\n");
}
