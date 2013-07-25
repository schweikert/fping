dnl Process this file with autoconf to produce a configure script.

dnl Minimum Autoconf version required.
AC_PREREQ(2.59)

AC_INIT([fping],[3.6-rc2])

dnl make ipv4 and ipv6 options
AC_ARG_ENABLE([ipv4],
  [  --enable-ipv4    Build IPv4 capable fping],
  [case "${enableval}" in
   yes)	ipv4=true ;;
   no)	ipv4=false ;;
   *)	AC_MSG_ERROR([bad value ${enableval} for --enable-ipv4]) ;;
   esac],[ipv4=true])
  AM_CONDITIONAL([IPV4], [test x$ipv4 = xtrue])

AC_ARG_ENABLE([ipv6],
  [  --enable-ipv6    Build IPv6 capable fping6],
  [case "${enableval}" in
   yes)	ipv6=true ;;
   no)	ipv6=false ;;
   *)	AC_MSG_ERROR([bad value ${enableval} for --enable-ipv6]) ;;
   esac],[ipv6=false])
  AM_CONDITIONAL([IPV6], [test x$ipv6 = xtrue])

if test x$ipv4 = xfalse && test x$ipv6 = xfalse; then
  AC_MSG_ERROR([You must enable at least one of IPv4 and IPv6.])
fi

AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE([foreign])
AM_MAINTAINER_MODE

AC_CONFIG_HEADERS([config.h])

dnl Checks for programs.

AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL

dnl Checks for libraries.

AC_CHECK_FUNC(gethostbyname)
if test $ac_cv_func_gethostbyname = no; then
    AC_CHECK_LIB(nsl, gethostbyname)
fi
AC_CHECK_FUNC(connect)
if test $ac_cv_func_connect = no; then
    AC_CHECK_LIB(socket, connect)
fi

AH_TOP([
#ifndef CONFIG_H
#define CONFIG_H
])

AH_BOTTOM([
/* some OSes do not define this ... lets take a wild guess */

#ifndef INADDR_NONE
#  define INADDR_NONE 0xffffffffU
#endif

#endif /* CONFIG_H */

])

dnl Checks for header files.
AC_CHECK_HEADERS(unistd.h sys/file.h stdlib.h sys/select.h)

AC_CONFIG_FILES([Makefile
                 doc/Makefile
                 src/Makefile])

AC_OUTPUT
