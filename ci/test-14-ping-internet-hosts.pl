#!/usr/bin/perl -w

use Test::Command tests => 3;

# fping
{
my $cmd = Test::Command->new(cmd => "fping -q -i 10 -p 20 -c 3 -t200  8.8.8.8 4.2.2.5 www.france-telecom.fr www.google.com");
$cmd->exit_is_num(0);
$cmd->stdout_is_eq("");
$cmd->stderr_like(qr{8\.8\.8\.8\s*: xmt/rcv/%loss = [123]/3/\d+%, min/avg/max = \d+\.\d+/\d+\.\d+/\d+\.\d+
4\.2\.2\.5\s*: xmt/rcv/%loss = [123]/3/\d+%, min/avg/max = \d+\.\d+/\d+\.\d+/\d+\.\d+
www\.france-telecom\.fr\s*: xmt/rcv/%loss = [123]/3/\d+%, min/avg/max = \d+\.\d+/\d+\.\d+/\d+\.\d+
www\.google\.com\s*: xmt/rcv/%loss = [123]/3/\d+%, min/avg/max = \d+\.\d+/\d+\.\d+/\d+\.\d+
});
}
