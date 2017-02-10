#!/usr/bin/perl -w

use Test::Command tests => 9;

my $I_HELP = "   -I, --iface=IFACE  bind to a particular interface\n";
$I_HELP = '' if $^O eq 'darwin';

# fping -h
my $cmd1 = Test::Command->new(cmd => "fping -h");
$cmd1->exit_is_num(0);
$cmd1->stdout_is_eq(<<END);
Usage: fping [options] [targets...]

Probing options:
   -4                 only ping IPv4 addresses
   -6                 only ping IPv6 addresses
   -b, --size=BYTES   amount of ping data to send, in bytes (default 56)
   -B, --backoff=N    set exponential backoff factor to N
   -c, --count=N      count of pings to send to each target (default 1)
   -f, --file=FILE    read list of targets from a file ( - means stdin)
   -g, --generate     generate target list (only if no -f specified)
                      (specify the start and end IP in the target list, or use a CIDR address)
                      (ex. fping -g 192.168.1.0 192.168.1.255 or fping -g 192.168.1.0/24)
   -H, --ttl=N        set the IP TTL value (Time To Live hops)
$I_HELP   -l, --loop         loop sending pings forever
   -m, --all          use all IPs of provided hostnames (e.g. IPv4 and IPv6), use with -A
   -M, --dontfrag     set the Don't Fragment flag
   -O, --tos=N        set the type of service (tos) flag on the ICMP packets
   -p, --period=MSEC  interval between ping packets to one target (in millisec)
                      (in looping and counting modes, default 1000)
   -r, --retry=N      number of retries (default 3)
   -R, --random       random packet data (to foil link data compression)
   -S, --src=IP       set source address
   -t, --timeout=MSEC individual target initial timeout (in millisec) (default 500)

Output options:
   -a, --alive        show targets that are alive
   -A, --addr         show targets by address
   -C, --vcount=N     same as -c, report results in verbose format
   -D, --timestamp    print timestamp before each output line
   -e, --elapsed      show elapsed time on return packets
   -i, --interval=MSEC  interval between sending ping packets (in ms) (default 25)
   -n, --name         show targets by name (-d is equivalent)
   -N, --netdata      output compatible for netdata (-l -Q are required)
   -o, --outage       show the accumulated outage time (lost packets * packet interval)
   -q, --quiet        quiet (don't show per-target/per-ping results)
   -Q, --squiet=SECS  same as -q, but show summary every n seconds
   -s, --stats        print final stats
   -u, --unreach      show targets that are unreachable
   -v, --version      show version
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
$cmd3->stderr_like(qr{^fping: (illegal|invalid) option -- '?Z'?\nsee 'fping -h' for usage information\n$});
