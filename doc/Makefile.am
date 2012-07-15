man_MANS = fping.8
EXTRA_DIST = $(man_MANS)

fping.8: fping.pod
	pod2man $< >$@
	pod2man  -c "" -s 8 -r "fping $(VERSION)" $< >$@
