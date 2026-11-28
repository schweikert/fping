dnl Process this file with autoconf to produce a configure script.

dnl Minimum Autoconf version required.
AC_PREREQ(2.59)

AC_INIT([fping],[5.6-rc1])
dnl Keep auxiliary scripts in top srcdir even when built inside nested worktrees.
AC_CONFIG_AUX_DIR([.])

m4_ifdef([AC_AUTOCONF_VERSION],[AC_USE_SYSTEM_EXTENSIONS], [AC_GNU_SOURCE])

dnl Detect IBM i PASE environment
AC_MSG_CHECKING([for IBM i PASE environment])
AC_COMPILE_IFELSE(
  [AC_LANG_PROGRAM([[]], [[
#if !defined(__PASE__)
#error not PASE
#endif
  ]])],
  [
    AC_MSG_RESULT([yes])
    AC_MSG_NOTICE([IBM i PASE detected: applying required CPPFLAGS and library paths])
    CPPFLAGS="-I/QOpenSys/pkgs/include -D_SIGSET_T -DHAVE_STDLIB_H $CPPFLAGS"
    LDFLAGS="-L/QOpenSys/pkgs/lib $LDFLAGS"
  ],
  [AC_MSG_RESULT([no])]
)

dnl Solaris/illumos hides POSIX msghdr.msg_control and CMSG_* unless XPG4.2/SUSv3 is enabled.
AC_CHECK_MEMBER([struct msghdr.msg_control], [], [
  AC_DEFINE([_XOPEN_SOURCE], [600], [Define if needed to expose struct msghdr.msg_control])
], [#include <sys/types.h>
#include <sys/socket.h>])

dnl Check for IP_RECVTOS (missing on PASE/IBM i and some BSDs).
AC_CHECK_DECL([IP_RECVTOS],
  [AC_DEFINE([HAVE_IP_RECVTOS], [1], [IP_RECVTOS socket option is available])],
  [],
  [#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>])

dnl --disable-ipv4
AC_ARG_ENABLE([ipv4],
  AS_HELP_STRING([--disable-ipv4], [Disable support for pinging IPv4 hosts]))
AM_CONDITIONAL([IPV4], [test "x$enable_ipv4" != "xno"])
AM_COND_IF([IPV4], [AC_DEFINE([IPV4], [1], [IPv4 enabled])])

dnl --disable-ipv6
AC_ARG_ENABLE([ipv6],
  AS_HELP_STRING([--disable-ipv6], [Disable support for pinging IPv6 hosts]))
AS_IF([test "x$enable_ipv6" != "xno"], [
    dnl Test if IPv6 is supported
       AC_CHECK_HEADERS([netinet/icmp6.h], [have_ipv6="yes"], [have_ipv6="no"], [[
      #include <netinet/in.h>
      #include <sys/types.h>
  ]])
])
AS_IF([test "x$have_ipv6" = "xyes"], [
    dnl Test if IPv6 is supported
       AC_CHECK_HEADERS([netinet/ip6.h], [have_ipv6="yes"], [have_ipv6="no"], [[
      #include <netinet/in.h>
      #include <sys/types.h>
  ]])
])
dnl Can't disable both IPv4 and IPv6
AS_IF([test "x$enable_ipv4" = "xno" -a "x$enable_ipv6" = "xno"], [
    AC_MSG_ERROR([Need to enable IPv4 or IPv6. Can't disable both!)])
])
dnl IPv6 required, but not supported?
AS_IF([test \( "x$enable_ipv6" = "xyes" -o "x$enable_ipv4" = "xno" \) -a "x$have_ipv6" != "xyes" ], [
    AC_MSG_ERROR([IPv6 not supported on this platform (netinet/icmp6.h header not found)])
])
AM_CONDITIONAL([IPV6], [test "x$have_ipv6" = "xyes"])
AM_COND_IF([IPV6], [AC_DEFINE([IPV6], [1], [IPv6 enabled])])

AC_ARG_ENABLE([timestamp],
  AS_HELP_STRING([--disable-timestamp], [Disable kernel-based packet timestaping (SO_TIMESTAMPNS)]))
AS_IF([test "x$enable_timestamp" != "xno"], [
   AC_CHECK_DECL([SO_TIMESTAMPNS], [AC_DEFINE(HAVE_SO_TIMESTAMPNS, [1], [SO_TIMESTAMPNS is defined])], [have_so_timestamp="no"], [#include <sys/types.h>
#include <sys/socket.h>])
])
dnl Test if --enable-timestamp is explicitely enabled and make an error if this platform doesn't support it
AS_IF([test "x$enable_timestamp" = "xyes" -a "x$have_so_timestamp" = "xno"], [
  AC_MSG_ERROR([--enable-timestamp not supported on this platform])
])

AC_ARG_ENABLE([safe-limits],
  AS_HELP_STRING([--enable-safe-limits], [Restrict timing parameters (-i, -p) within "safe" limits]))
AS_IF([test "x$enable_safe_limits" = "xyes"], [
   AC_DEFINE(FPING_SAFE_LIMITS, [1], [safe limits should be enforced])])

AC_ARG_ENABLE([debug],
  AS_HELP_STRING([--enable-debug], [enable debugging @<:@default=no@:>@]), [enable_debug=$enableval], [enable_debug=no])
AS_IF([test "x$enable_debug" = "xyes"], [
  AC_DEFINE([DEBUG], [1], [Define if debugging is enabled])])

AC_ARG_ENABLE([werror],
  AS_HELP_STRING([--enable-werror], [Treat compiler warnings as errors @<:@default=no@:>@]), [enable_werror=$enableval], [enable_werror=no])

AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AM_MAINTAINER_MODE

AC_CONFIG_HEADERS([config.h])

dnl Checks for programs.

AC_PROG_CC
AM_PROG_CC_C_O
m4_version_prereq([2.70],,[AC_PROG_CC_STDC])
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
AC_CHECK_FUNC(sigaction)
if test $ac_cv_func_sigaction = yes; then
    AC_DEFINE([USE_SIGACTION],[1],[Define if sigaction is available.])
fi

AC_CHECK_FUNCS([strftime], [],
  [AC_MSG_ERROR([strftime function is required but not found])])

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
AC_CHECK_HEADERS([unistd.h sys/file.h stdlib.h sys/select.h])

AC_CHECK_DECL([MSG_DONTWAIT], [AC_DEFINE(HAVE_MSG_DONTWAIT, [1], [MSG_DONTWAIT is defined])], [], [#include <sys/types.h>
#include <sys/socket.h>])

dnl Probe compiler warning flags (GCC and Clang set GCC=yes).
dnl Keep compatible with AC_PREREQ(2.59): no AX_CHECK_COMPILE_FLAG or AS_VAR_IF.
dnl Omit user CFLAGS during probing so unrelated user flags (e.g. -w or linker
dnl flags on Clang) cannot poison -Werror probes, and use main(void) so
dnl -Wstrict-prototypes -Werror succeeds on valid flags.
AC_DEFUN([FPING_APPEND_WARN_CFLAG], [
  if test -z "$WARN_CFLAGS"; then
    WARN_CFLAGS="$1"
  else
    WARN_CFLAGS="$WARN_CFLAGS $1"
  fi
])

AC_DEFUN([FPING_CHECK_CFLAG], [
  AC_MSG_CHECKING([whether $CC supports $1])
  fping_save_CFLAGS="$CFLAGS"
  CFLAGS="$WARN_CFLAGS -Werror $1"
  AC_COMPILE_IFELSE(
    [AC_LANG_SOURCE([[int main(void) { return 0; }]])],
    [fping_cflag_supported=yes],
    [fping_cflag_supported=no])
  CFLAGS="$fping_save_CFLAGS"
  AC_MSG_RESULT([$fping_cflag_supported])
  if test "x$fping_cflag_supported" = "xyes"; then
    m4_if([$2], [], [FPING_APPEND_WARN_CFLAG([$1])], [$2])
  fi
])

WARN_CFLAGS=""
fping_werror_ok=no
if test "x$GCC" = "xyes"; then
  FPING_CHECK_CFLAG([-Wall])
  FPING_CHECK_CFLAG([-Wextra])
  FPING_CHECK_CFLAG([-Wsign-compare], [FPING_APPEND_WARN_CFLAG([-Wno-sign-compare])])
  FPING_CHECK_CFLAG([-Wstrict-prototypes])
  FPING_CHECK_CFLAG([-Wold-style-definition])
  FPING_CHECK_CFLAG([-Wmissing-prototypes])
  FPING_CHECK_CFLAG([-Wshadow])
  FPING_CHECK_CFLAG([-Wformat-security])
  FPING_CHECK_CFLAG([-Wformat-nonliteral])
  FPING_CHECK_CFLAG([-Wimplicit-fallthrough])
  FPING_CHECK_CFLAG([-Wundef])
  FPING_CHECK_CFLAG([-Wvla])
  if test "x$enable_werror" = "xyes"; then
    FPING_CHECK_CFLAG([-Werror], [
      FPING_APPEND_WARN_CFLAG([-Werror])
      fping_werror_ok=yes
    ])
  fi
fi
if test "x$enable_werror" = "xyes" && test "x$fping_werror_ok" != "xyes"; then
  AC_MSG_ERROR([--enable-werror is not supported by this compiler])
fi
AC_SUBST([WARN_CFLAGS])

AC_CONFIG_FILES([Makefile
                 doc/Makefile
                 src/Makefile])

AC_OUTPUT
