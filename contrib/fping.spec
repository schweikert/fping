Summary: send ICMP echo probes to multiple hosts
Name: fping
Version: 3.4
Release: 1
License: MIT
Group: Applications/System
Source0: http://fping.org/dist/%{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-buildroot

%description
fping is a program to send ICMP echo probes to network hosts, similar to ping,
but much better performing when pinging multiple hosts. fping has a very long
history: Roland Schemers did publish a first version of it in 1992 and it has
established itself since then as a standard tool for network diagnostics and
statistics.

%prep
%setup -q

%build

if [ ! -f ./configure ] ; then
    ./autogen.sh
fi

# fping
%configure --enable-ipv4
make

# fping6
%configure --enable-ipv6
make
%{__mv} -f src/fping src/fping6

%install
rm -rf $RPM_BUILD_ROOT
make DESTDIR=$RPM_BUILD_ROOT install

# fping6
%{__install} -Dp -m4755 src/fping6 %{buildroot}%{_sbindir}/fping6
%{__ln_s} -f fping.8 %{buildroot}%{_mandir}/man8/fping6.8

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
%attr(4755, root, root) /usr/sbin/fping
%attr(4755, root, root) /usr/sbin/fping6
%doc README COPYING ChangeLog
/usr/share/man/man8/fping.8.gz
/usr/share/man/man8/fping6.8.gz

%post
if [ -x /usr/sbin/setcap ]; then
    /bin/chmod 0755 /usr/sbin/fping*
    /usr/sbin/setcap cap_net_raw+ep /usr/sbin/fping*
fi

%changelog
* Mon Dec 24 2012 Marcus Vinicius Ferreira <ferreira.mv@gmail.com>
- Missing './configure' script when cloning from master.
- Making 'fping6'.
- Fix setuid permission to 'rwsr-xr-x'.
- doc files.
- Replacing setuid permission if 'setcap' is present on post-install.
- Using 'http://fping.org/dist/' for release source distributions.

* Mon Jul 16 2012 Stephen Schaefer <sschaefer@acm.org>
- Initial build

# vim:ft=spec:

