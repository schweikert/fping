man_MANS = fping.8

EXTRA_DIST = fping.8 fping.pod README.1992

fping.8: fping.pod
	pod2man  -c "" -s 8 -r "fping" $< >$@
