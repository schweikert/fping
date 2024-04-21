fping 5.2 (2024-04-21)
======================

## New features

- New option -X / --fast-reachable to exit immediately once N hosts have been
  found (#260, thanks @chriscray and @gsnw)

- New option -k / -fwmark to set Linux fwmark mask (#289, thanks @tomangert and
  @deepkv)

## Bugfixes and other changes

- Always output fatal error messages (#303, thanks @auerswal)
- Fallback to SO\_TIMESTAMP if SO\_TIMESTAMPNS is not available (#279, thanks
  @gsnw)
- Fix "not enough sequence numbers available" error on BSD-like systems (#307,
  thanks @cagney, @gsnw)
- Fix running in unprivileged mode (#248, thanks @sfan5)
- Fix build issue for NetBSD/alpha (#255, thanks @0-wiz-0)
- Fix build issue for OpenBSD/alpha (#275, thanks @gsnw)
- Fix build warning for long int usage (#258, thanks @gsnw)
- Fix build error with musl libc (#263, thanks @kraj)
- Fix to guard against division by zero (#293, thanks @auerswal)
- Decouple -a/-u effects from -c (#298, thanks @auerswal)
- Added contrib/Dockerfile (#224, thanks @darless)
- Remove host from Netdata chart titles (#253, thanks @ilyam8)
- Add additional tests (#292, #297, thanks @auerswal)
- Update github action os images (#282, thanks @gsnw)
- Fix Azure pipeline tests (#308, thanks @gsnw)
- Various autoconf fixes (#286, #283, thanks @gsnw)
- Extended configure script with --enable-debug and output cpu usage (#311,
  thanks @gsnw)
- Documentation: Update Netdata website link (#257, thanks @ilyam8)
- Documentation: fix description of --file option (#268, thanks @MohGeek)
- Documentation: improve exit status description (#294, thanks @auerswal)
- Documentation: move description of -i MSEC (#298, thanks @auerswal)
- Documentation: improve help output for options -c and -C (#302, #auerswal)


fping 5.1 (2022-02-06)
======================

## Bugfixes and other changes

- Use setcap to specify specific files in fping.spec (#232, thanks @zdyxry)
- Netdata: use host instead name as family label (#226, thanks @k0ste)
- Netdata: use formatstring macro PRId64 (#229, thanks @gsnw)
- Allow -4 option to be given multiple times (#215, thanks @normanr)
- Documentation fix (#208, thanks @timgates42)
- Retain privileges until after privileged setsockopt (#200, thanks @simetnicbr)
- Set bind to source only when option is set (#198, thanks @dinoex)
- Update Azure test pipeline (#197, thanks @gsnw)
- Fix getnameinfo not called properly for IPv4 (#227, thanks @aafbsd)
- Fixed wrong timestamp under Free- and OpenBSD and macOS (#217, thanks @gsnw)
- Documentation updates (#240, thanks @auerswal)
- Updated autotools (autoconf 2.71, automake 1.16.5, libtool 2.4.6)


fping 5.0 (2020-08-05)
======================

## Incompatible Changes

- In non-quiet loop and count mode, a line is printed for every lost packet
  (#175, thanks @kbucheli):

  ```
  $ fping -D -c2 8.8.8.8 8.8.8.7
  [1596092373.18423] 8.8.8.8 : [0], 64 bytes, 12.8 ms (12.8 avg, 0% loss)
  [1596092374.18223] 8.8.8.7 : [0], timed out (NaN avg, 100% loss)
  [1596092374.18424] 8.8.8.8 : [1], 64 bytes, 12.3 ms (12.5 avg, 0% loss)
  [1596092375.18344] 8.8.8.7 : [1], timed out (NaN avg, 100% loss)

  8.8.8.8 : xmt/rcv/%loss = 2/2/0%, min/avg/max = 12.3/12.5/12.8
  8.8.8.7 : xmt/rcv/%loss = 2/0/100%
  ```

- The returned size in bytes now always excludes the IP header, so if before it
  reported '84 bytes' e.g. when using 'fping -l', now it reports '64 bytes'.
  This is to make the reported size consistent with ping(8) from iputils and
  also with fping when pinging a IPv6 host (which never included the IPv6
  header size).

## New features

- The number of sent pings is only counted when the pings are received or have
  timed out, ensuring that the loss ratio will be always correct. This makes it
  possible, for example, to use loop mode (-l) with interval statistics (-Q)
  and a timeout larger than period, without having the issue that initially
  some pings would be reported as missing (#193)

- Improved precision of measurements from 10us to 1us (#136, thanks @tycho)

## Bugfixes and other changes

- The reported size of received packets is now always correct on Linux even for
  packets > 4096 bytes (#180)

- Travis CI automated testing now also macos testing and additional ubuntu
  distributions (#196)

fping 4.4 (2020-07-24)
======================
## Bugfixes and other changes

- Fix wrong ident used for normal (non-unprivileged) pings (#191, thanks @tycho)
- Fix build with --disable-ipv6 (#187, thanks Polynomial-C)

fping 4.3 (2020-07-11)
======================

## New features

- Linux unprivileged ping support (#173, thanks @tycho)
- Add SIGQUIT summary support similar to ping (#185, thanks @laddp)

## Bugfixes and other changes

- Corrected long option name of -s to --stats (#148, thanks @wopfel)
- Do not fail if using fping6 with -6 flag (#149, thanks @stromnet)
- Fail if interface binding (-I) does not work (#162, thanks @kbucheli)
- Fix using option -4 when fping is compiled IPv4-only (#154, thanks @pbhenson)
- Add Azure pipeline test build (#153 and #170, thanks @gsnw)
- GCC 10 compatibility fixes (#167 and #168, thanks @cranderson)
- Macos build fix (#174, thanks @tycho)
- Fix xmt stats in Netdata output (#172, thanks @vlvkobal)
- Only increase num_alive if response is not a duplicate (#151, thanks @brownowski)
- Use line buffering for stdout (#179, thanks @bg6cq)

fping 4.2 (2019-02-19)
======================

## New features

- New option -x / --reachable to check if the number of reachable hosts is >= a certain
  number. Useful for example to implement connectivity-checks (#138, thanks @deepak0004)

## Bugfixes and other changes

- Allow decimal numbers for '-t', '-i', '-p', and '-Q'
- Fix build with --disable-ipv6 (#134, thanks @Polynomial-C)
- Fix hang with '-6', with ipv6 kernel module, but not loaded (#140, thanks @abelbeck)
- Assume '-6' if the binary is named 'fping6' (this is mostly for special
  embedded-distro use cases, and not meant to be used generally in place of
  compiling IPv6-only binary or using '-6', see also the notes in #139, thanks
  abelbeck)
- Get rid of warning "timeout (-t) value larger than period (-p) produces unexpected results"
  (#142, thanks @MrDragon1122)
  

fping 4.1 (2018-09-17)
======================

## Bugfixes and other changes

- Fix problem when socket fd is 0 (#125, thanks Ramón Novoa!)
- Fix running on servers with disabled IPv6 (#118, thanks Simon Matter)
- Allow running "fping -h" or "--help" even when raw socket can't be opened (#131, thanks @teto)
- Fix build issue with FreeBSD and IPv6 (#132, thanks @gsnw)

fping 4.0 (2017-04-23)
======================

## Incompatible Changes

##### fping and fping6 unification

fping and fping6 are now unified into one binary. It means that, for example,
doing 'fping google.com' is going to ping the IPv6 IP of google.com on
IPv6-enabled hosts.  

If you need exact compatibility with old versions, you can configure and
install fping twice: once for ipv4, and once for ipv6:

    ./configure --disable-ipv6; make clean install
    ./configure --disable-ipv4 --program-suffix=6; make clean install

##### Option -n, not the same as -d anymore

Option -n / --name is now doing a reverse-DNS lookups on host addresses,
only if they are given as IP address, but not for hostnames. For example,
if you write 'fping -n google.com', fping would previously do a
forward-DNS lookup on google.com, and then a reverse-DNS lookup on the
resolved IP address. Now, it is just going to keep the name 'google.com'.
That same behavior can be achieved with the option -d / --rdns (which was
previously an alias for -n).

                     fping<4.0              fping>=4.0
    fping -n NAME    NAME->IP->IPNAME       NAME
    fping -d NAME    NAME->IP->IPNAME       NAME->IP->IPNAME

##### Discarding of late packets

fping will now discard replies, if they arrive after the defined timeout
for reply packets, specified with -t. This change is relevant only for the
count and loop modes, where the measured times should be now more
consistent (see github issue [#32][i32] for details).

To prevent loosing reply packets because of this change, the default
timeout in count and loop modes is now automatically adjusted to the
period interval (up to 2000 ms), but it can be overriden with the -t
option. The default timeout for non-loop/count modes remains 500 ms.

##### No restrictions by default

fping will not enforce -i >= 1 and -p >= 10 anymore, except if you
'./configure --enable-safe-limits'.

The reasoning to removing the restrictions by default, is that users can
clog the network with other tools anyway, and these restrictions are
sometimes getting in the way (for example if you try to ping a lot of
hosts).

##### Default interval (-i) changed from 25ms to 10ms

The default minimum interval between ping probes has been changed from
25ms to 10ms. The reason is that 25ms is very high, considering today's
fast networks: it generates at most 31 kbps of traffic (for IPv4 and
default payload size).

## New features

- Unified 'fping' and 'fping6' into one binary ([#80][i80])
- Long option names for all options
- IPv6 enabled by default
- New option -4 to force IPv4
- New option -6 to force IPv6
- Keep original name if a hostname is given with -n/--name
- Option -d/--rdns now always does a rdns-lookup, even for names, as '-n' was doing until now
- Enforce -t timeout on reply packets, by discarding late packets ([#32][i32])
- Auto-adjust timeout for -c/-C/-l mode to value of -p

## Bugfixes and other changes

- -i/-p restrictions disabled by default (enable with --enable-safe-limits)
- Default interval -i changed from 25ms to 10ms
- Fix compatibility issue with GNU Hurd
- A C99 compiler is now required
- Option parsing with optparse (https://github.com/skeeto/optparse). Thanks Christopher Wellons!
- New changelog file format

[i32]: https://github.com/schweikert/fping/issues/32
[i80]: https://github.com/schweikert/fping/issues/80

(see doc/CHANGELOG.pre-v4 for older changes)
