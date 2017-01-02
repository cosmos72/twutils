#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.65])
AC_INIT([twxmms2], [0.5.0])
AC_CONFIG_SRCDIR([src/twxmms2.c])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([1.14 foreign -Wall -Werror])

AM_MAINTAINER_MODE        dnl by default, do not try to run libtoolize/automake/autoconf/autoreconf because build system may lack them

AC_LANG_C
AC_PROG_CC
AC_PROG_CPP
AC_PROG_MAKE_SET
AC_PROG_INSTALL

AC_HEADER_STDC

AC_CHECK_PROG(pkg_config, pkg-config, yes)
if test "$ac_cv_prog_pkg_config" != "yes"; then
  AC_MSG_ERROR([your system is missing 'pkg-config' !
Check, if you installed the pkg-config package correctly])
fi


if pkg-config xmms2-client --cflags >/dev/null 2>&1 && pkg-config xmms2-client --libs >/dev/null 2>&1
then
  CPPFLAGS="$CPPFLAGS `pkg-config xmms2-client --cflags`"
  LIBS="$LIBS `pkg-config xmms2-client --libs`"
else
  AC_MSG_ERROR([your system is missing 'pkg-config xmms2-client' !
 Check, if you installed the xmms2-client package and header files correctly])
fi

AC_CHECK_HEADERS([unistd.h sys/wait.h xmmsclient/xmmsclient.h Tw/Tw.h])
AC_CHECK_FUNCS([getenv waitpid nanosleep sleep])

if test "$ac_cv_header_xmmsclient_xmmsclient_h" = "no"; then
  AC_MSG_ERROR([your system is missing xmmsclient/xmmsclient.h header!
Check, if you installed the xmms2-client header files correctly.])
fi

if test "$ac_cv_header_Tw_Tw_h" = "no"; then
  AC_MSG_ERROR([your system is missing Tw/Tw.h header!
Check, if you installed the libTw header files correctly.])
fi

dnl Checks for libraries.

AC_MSG_CHECKING([whether the libxmmsclient library can be linked])
AC_LINK_IFELSE([
  AC_LANG_PROGRAM([
    #include <xmmsclient/xmmsclient.h>
  ], [
    xmmsc_connection_t * conn = xmmsc_init ("twxmms2");
    int err = xmmsc_connect (conn, "");                  
])],
  [AC_MSG_RESULT(yes)],
  [AC_MSG_RESULT(no)]
  [AC_MSG_ERROR([Your system fails linking a small libxmmsclient application!
Check, if your compiler is installed correctly and if you have used the
same compiler to compile libxmmsclient as you did use now])])


LIBS="$LIBS -lTw"

AC_MSG_CHECKING([whether the libTw library can be linked])
AC_LINK_IFELSE([
  AC_LANG_PROGRAM([
    #include <Tw/Tw.h>
  ], [
    tdisplay d = Tw_Open("");
])], [AC_MSG_RESULT(yes)],
  [AC_MSG_RESULT(no)]
  [AC_MSG_ERROR([Your system fails linking a small libTw application!
Check, if your compiler is installed correctly and if you have used the
same compiler to compile libTw as you did use now])])


AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
