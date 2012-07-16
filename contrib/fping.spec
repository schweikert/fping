Summary: send ICMP echo probes to multiple hosts
Name: fping
Version: 3.2
Release: 1
License: MIT
Group: Applications/System
Source0: %{name}-%{version}.tar.gz
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
%configure

make

%install
rm -rf $RPM_BUILD_ROOT
make DESTDIR=$RPM_BUILD_ROOT install

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
/usr/sbin/fping
/usr/share/man/man8/fping.8.gz

%post
if [ -x /usr/sbin/setcap ]; then
    /usr/sbin/setcap cap_net_raw+ep /usr/sbin/fping
else
    chmod 1777 /usr/sbin/fping
fi

%changelog
* Mon Jul 16 2012 Stephen Schaefer <sschaefer@acm.org>
- Initial build
