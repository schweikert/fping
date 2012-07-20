man =

if IPV4
man += fping.8
endif
if IPV6
man += fping6.8
endif

man_MANS = $(man)
EXTRA_DIST = fping.pod README.1992


fping.8: fping.pod
	pod2man $< >$@
	pod2man  -c "" -s 8 -r "fping $(VERSION)" -n FPING $< >$@

fping6.8: fping.pod
	pod2man $< >$@
	pod2man -c "" -s 8 -r "fping $(VERSION)" -n FPING6 $< >$@
	${SED} -i.bak -e 's/fping/fping6/' fping6.8

clean:
	${RM} ${man_MANS} fping6.8.bak
