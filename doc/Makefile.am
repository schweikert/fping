man_MANS =

man_MANS += fping.8

if IPV6
man_MANS += fping6.8
endif

EXTRA_DIST = fping.8 fping6.8 fping.pod README.1992

fping.8: fping.pod
	pod2man  -c "" -s 8 -r "fping" $< >$@

fping6.8: fping.pod
	pod2man  -c "" -s 8 -r "fping" -n fping6 $< >$@
