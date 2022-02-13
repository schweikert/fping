#!/usr/bin/perl -w

use English;
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
my $fping_bin = `which fping`; chomp $fping_bin;
system("cp $fping_bin /tmp/fping.copy; chmod +x /tmp/fping.copy");

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
    plan tests => 3;

    {
        my $cmd = Test::Command->new(cmd => "fping 127.0.0.1");
        $cmd->exit_is_num(0);
        $cmd->stdout_is_eq("127.0.0.1 is alive\n");
        $cmd->stderr_is_eq("");
    }
}

sub test_privileged_fails {
    plan tests => 3;

    {
        my $cmd = Test::Command->new(cmd => "/tmp/fping.copy 127.0.0.1");
        $cmd->exit_is_num(4);
        $cmd->stdout_is_eq("");
        $cmd->stderr_like(qr{: can't create socket \(must run as root\?\)});
    }
}
