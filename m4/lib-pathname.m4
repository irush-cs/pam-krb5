dnl lib-pathname.m4 -- Determine the library path name.
dnl
dnl Red Hat systems and some other Linux systems use lib64 and lib32 rather
dnl than just lib in some circumstances.  This file provides an Autoconf
dnl macro, RRA_SET_LDFLAGS, which given a variable and a prefix, adds
dnl -Lprefix/lib, -Lprefix/lib32, or -Lprefix/lib64 to the variable depending
dnl on which directories exist and the size of a long in the compilation
dnl environment.
dnl
dnl Written by Russ Allbery <rra@stanford.edu>
dnl Copyright 2008 Board of Trustees, Leland Stanford Jr. University
dnl
dnl See LICENSE for licensing terms.

dnl Probe for the alternate library name that we should attempt on this
dnl architecture, given the size of an int, and set rra_lib_arch_name to that
dnl name.  Separated out so that it can be AC_REQUIRE'd and not run multiple
dnl times.
dnl
dnl There is an unfortunate abstraction violation here where we assume we know
dnl the cache variable name used by Autoconf.  Unfortunately, Autoconf doesn't
dnl provide any other way of getting at that information in shell that I can
dnl see.
AC_DEFUN([_RRA_LIB_ARCH_NAME],
[rra_lib_arch_name=lib
 AC_CHECK_SIZEOF([long])
 AS_IF([test "$ac_cv_sizeof_long" -eq 4 && test -d /usr/lib32],
     [rra_lib_arch_name=lib32],
     [AS_IF([test "$ac_cv_sizeof_long" -eq 8 && test -d /usr/lib64],
         [rra_lib_arch_name=lib64])])])

dnl The public interface.  Set VARIABLE to PREFIX/lib{,32,64} as appropriate.
AC_DEFUN([RRA_SET_LDFLAGS],
[AC_REQUIRE([_RRA_LIB_ARCH_NAME])
 AS_IF([test -d "$2/$rra_lib_arch_name"],
    [$1="-L$2/$rra_lib_arch_name"],
    [$1="-L$2/lib"])])