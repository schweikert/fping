#!/usr/bin/perl -w

use English;
use File::Temp qw/ tempdir /;
use Test::Command;
use Test::More;

if( $^O eq 'darwin' ) {
    plan skip_all => 'Test irrelevant on MacOS';
    exit 0;
}

sub get_ping_gid_range {
    open FD, "/proc/sys/net/ipv4/ping_group_range" or return undef;
    chomp(my $line = <FD>);
    my @range = split(/\s+/, $line);
    close FD;
    return @range;
}

my @gids = split(' ', $EGID);
my @allowed = get_ping_gid_range();

# Make a copy of the binary so that we get rid of setuid bit
my $tmpdir = tempdir(CLEANUP => 1);
my $fping_bin = `which fping`; chomp $fping_bin;
my $fping_copy = "$tmpdir/fping.copy";
system("cp $fping_bin $fping_copy; chmod +x $fping_copy");

# Determine what test to run, based on whether unprivileged
# pings are allowed.
if(scalar grep { $_ >= $allowed[0] && $_ <= $allowed[1] } @gids) {
    diag('test unprivileged mode');
    test_unprivileged_works();
}
else {
    test_privileged_fails();
}

sub test_unprivileged_works {
    plan tests => 12;

    {
        my $cmd = Test::Command->new(cmd => "$fping_copy 127.0.0.1");
        $cmd->exit_is_num(0);
        $cmd->stdout_is_eq("127.0.0.1 is alive\n");
        $cmd->stderr_is_eq("");
    }
    {
        my $cmd = Test::Command->new(cmd => "$fping_copy --print-tos 127.0.0.1");
        $cmd->exit_is_num(0);
        $cmd->stdout_is_eq("127.0.0.1 is alive (TOS unknown)\n");
        $cmd->stderr_is_eq("");
    }
    SKIP: {
        if($^O ne 'linux') {
            skip '-k option is only supported on Linux', 3;
        }
        my $cmd = Test::Command->new(cmd => "$fping_copy -4 -k 256 127.0.0.1");
        $cmd->exit_is_num(0);
        $cmd->stdout_is_eq("127.0.0.1 is alive\n");
        $cmd->stderr_like(qr{fwmark ipv4: .+\n});
    }
    SKIP: {
        if($^O ne 'linux') {
            skip '-k option is only supported on Linux', 3;
        }
        if($ENV{SKIP_IPV6}) {
            skip 'Skip IPv6 tests', 3;
        }
        my $cmd = Test::Command->new(cmd => "$fping_copy -6 -k 256 ::1");
        $cmd->exit_is_num(0);
        $cmd->stdout_is_eq("::1 is alive\n");
        $cmd->stderr_like(qr{fwmark ipv6: .+\n});
    }
}

sub test_privileged_fails {
    plan tests => 3;

    {
        my $cmd = Test::Command->new(cmd => "$fping_copy 127.0.0.1");
        $cmd->exit_is_num(4);
        $cmd->stdout_is_eq("");
        $cmd->stderr_like(qr{: can't create socket \(must run as root\?\)});
    }
}
