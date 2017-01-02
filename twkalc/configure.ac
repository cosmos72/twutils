AC_PREREQ([2.65])
AC_INIT([twkalc], [0.5])
AC_CONFIG_SRCDIR([src/kcalc.cpp])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([1.14 foreign -Wall -Werror])

AM_MAINTAINER_MODE        dnl by default, do not try to run libtoolize/automake/autoconf/autoreconf because build system may lack them

AC_PROG_CC
AC_PROG_CXX

AC_TYPE_LONG_DOUBLE
AC_CHECK_HEADERS([ieeefp.h])
AC_CHECK_FUNCS([isinf])
AC_CHECK_LIB(m, sqrt)
AC_CHECK_LIB(Tw, Tw_Open)

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
