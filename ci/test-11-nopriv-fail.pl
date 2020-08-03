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
my @allowed_gid_range = get_ping_gid_range();

# Linux test for unprivileged ping support
foreach(@gids) {
	if ($_ >= $allowed_gid_range[0] && $_ <= $allowed_gid_range[1]) {
		plan skip_all => "Userspace pings are allowed, gid $_ in range [$allowed_gid_range[0], $allowed_gid_range[1]]";
		exit 0;
	}
}

plan tests => 3;

# run without privileges
my $fping_bin = `which fping`; chomp $fping_bin;
system("cp $fping_bin /tmp/fping.copy; chmod +x /tmp/fping.copy");

# fping
{
my $cmd = Test::Command->new(cmd => "/tmp/fping.copy 127.0.0.1");
$cmd->exit_is_num(4);
$cmd->stdout_is_eq("");
$cmd->stderr_like(qr{: can't create socket \(must run as root\?\)});
}
