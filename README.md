[![Build Status](https://travis-ci.org/schweikert/fping.svg?branch=develop)](https://travis-ci.org/schweikert/fping)
[![Coverage Status](https://coveralls.io/repos/github/schweikert/fping/badge.svg?branch=develop)](https://coveralls.io/github/schweikert/fping?branch=develop)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/11559/badge.svg?flat=1")](https://scan.coverity.com/projects/schweikert-fping)

# fping

fping is a program to send ICMP echo probes to network hosts, similar to ping,
but much better performing when pinging multiple hosts. fping has a long long
story: Roland Schemers did publish a first version of it in 1992 and it has
established itself since then as a standard tool.

_Current maintainer_:  
  David Schweikert \<david@schweikert.ch\>

_Website_:  
  https://fping.org/

_Mailing-list_:  
  https://groups.google.com/group/fping-users

## Installation

If you want to install fping from source, proceed as follows:

0. Run `./autogen.sh`
   (only if you got the source from Github).
1. Run `./configure` with the correct arguments.
   (see: `./configure --help`)
2. Run `make; make install`.
3. Make fping either setuid, or, if under Linux:
   `sudo setcap cap_net_raw,cap_net_admin+ep fping`

If you can't run fping as root or can't use the cap_net_raw capability, you can
also run fping in unprivileged mode. This works on MacOS and also on Linux,
provided that your GID is included in the range defined in
`/proc/sys/net/ipv4/ping_group_range`. This is particularly useful for running
fping in rootless / unprivileged containers. The --fwmark option needs root or
cap_net_admin. setuid will not work for --fwmark.

## Usage

Have a look at the [fping(8)](doc/fping.pod) manual page for usage help.
(`fping -h` will also give a minimal help output.)

## Credits

* Original author:  Roland Schemers (schemers@stanford.edu)
* Previous maintainer:  RL "Bob" Morgan (morgan@stanford.edu)
* Initial IPv6 Support: Jeroen Massar (jeroen@unfix.org / jeroen@ipng.nl)
* Other contributors: see [CHANGELOG.md](CHANGELOG.md)
