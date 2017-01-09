#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.65])
AC_INIT([twplayer_remote], [0.5.0])
AC_CONFIG_SRCDIR([src/player.c])
AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_AUX_DIR([admin])
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


if pkg-config dbus-1 --cflags >/dev/null 2>&1 && pkg-config dbus-1 --libs >/dev/null 2>&1
then
  CPPFLAGS="$CPPFLAGS `pkg-config dbus-1 --cflags`"
  LIBS="$LIBS `pkg-config dbus-1 --libs`"
else
  AC_MSG_ERROR([your system is missing 'pkg-config dbus-1' !
 Check, if you installed the dbus-1 package and header files correctly])
fi

AC_CHECK_HEADERS([unistd.h sys/wait.h dbus/dbus.h Tw/Tw.h])
AC_CHECK_FUNCS([getenv waitpid nanosleep sleep])

if test "$ac_cv_header_dbus_dbus_h" = "no"; then
  AC_MSG_ERROR([your system is missing dbus/dbus.h header!
Check, if you installed the dbus-1 header files correctly.])
fi

if test "$ac_cv_header_Tw_Tw_h" = "no"; then
  AC_MSG_ERROR([your system is missing Tw/Tw.h header!
Check, if you installed the libTw header files correctly.])
fi

dnl Checks for libraries.

AC_MSG_CHECKING([whether the libdbus library can be linked])
AC_LINK_IFELSE([
  AC_LANG_PROGRAM([
    #include <dbus/dbus.h>
  ], [
    DBusError * error = NULL;
    DBusConnection* conn = dbus_connection_open("", error);
])],
  [AC_MSG_RESULT(yes)],
  [AC_MSG_RESULT(no)]
  [AC_MSG_ERROR([Your system fails linking a small libdus application!
Check, if your compiler is installed correctly and if you have used the
same compiler to compile libdbus as you did use now])])


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
