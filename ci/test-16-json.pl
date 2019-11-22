#!/usr/bin/perl -w

use Test::Command;
use Test::More;

plan tests => 18;

# summary
{
my $cmd = Test::Command->new(cmd => "fping -c 2 -J 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr{\{
  "hosts": \{
    "127\.0\.0\.1": \{
      "xmt": 2,
      "rcv": 2,
      "loss_percentage": 0,
      "min": \d.\d+,
      "avg": \d.\d+,
      "max": \d.\d+
    \}
  \}
\}}
);
$cmd->stderr_is_eq("");
}

# all RTTs
{
my $cmd = Test::Command->new(cmd => "fping -C 2 -J 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr{\{
  "hosts": \{
    "127\.0\.0\.1": \[
      \d.\d+,
      \d.\d+
    \]
  \}
\}}
);
$cmd->stderr_is_eq("");
}

# summary with stats and outage
{
my $cmd = Test::Command->new(cmd => "fping -c 2 -s -o -J 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr{\{
  "hosts": \{
    "127\.0\.0\.1": \{
      "xmt": 2,
      "rcv": 2,
      "loss_percentage": 0,
      "outage": 0,
      "min": \d.\d+,
      "avg": \d.\d+,
      "max": \d.\d+
    \}
  \},
  "stats": \{
    "targets": 1,
    "alive": 1,
    "unreachable": 0,
    "unknown_addresses": 0,
    "timeouts": 0,
    "icmp_echos_sent": 2,
    "icmp_echo_replies_received": 2,
    "other_icmp_received": 0,
    "min_rtt": \d.\d+,
    "avg_rtt": \d.\d+,
    "max_rtt": \d.\d+,
    "elapsed_real_time": \d.\d+
  \}
\}}
);
$cmd->stderr_is_eq("");
}

# all RTTs with stats
{
my $cmd = Test::Command->new(cmd => "fping -C 2 -s -J 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr{\{
  "hosts": \{
    "127\.0\.0\.1": \[
      \d.\d+,
      \d.\d+
    \]
  \},
  "stats": \{
    "targets": 1,
    "alive": 1,
    "unreachable": 0,
    "unknown_addresses": 0,
    "timeouts": 0,
    "icmp_echos_sent": 2,
    "icmp_echo_replies_received": 2,
    "other_icmp_received": 0,
    "min_rtt": \d.\d+,
    "avg_rtt": \d.\d+,
    "max_rtt": \d.\d+,
    "elapsed_real_time": \d.\d+
  \}
\}}
);
$cmd->stderr_is_eq("");
}

# more indentation
{
my $cmd = Test::Command->new(cmd => "fping -c 2 --json=4 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr{\{
    "hosts": \{
        "127\.0\.0\.1": \{
            "xmt": 2,
            "rcv": 2,
            "loss_percentage": 0,
            "min": \d.\d+,
            "avg": \d.\d+,
            "max": \d.\d+
        \}
    \}
\}}
);
$cmd->stderr_is_eq("");
}

# no pretty-print
{
my $cmd = Test::Command->new(cmd => "fping -c 2 --json=0 127.0.0.1");
$cmd->exit_is_num(0);
$cmd->stdout_like(qr{\{"hosts":\{"127\.0\.0\.1":\{"xmt":2,"rcv":2,"loss_percentage":0,"min":\d.\d+,"avg":\d.\d+,"max":\d.\d+\}\}\}});
$cmd->stderr_is_eq("");
}
