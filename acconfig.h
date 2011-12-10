#ifndef CONFIG_H
#define CONFIG_H

@TOP@

/* VERSION (from configure.in) */
#undef VERSION


@BOTTOM@

/* some OSes do not define this ... lets take a wild guess */

#ifndef INADDR_NONE
#  define INADDR_NONE 0xffffffffU
#endif

#endif /* CONFIG_H */
