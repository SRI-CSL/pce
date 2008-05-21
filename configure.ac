dnl
dnl Configure for MCSAT
dnl
dnl This builds a platform-dependent configuration
dnl file stored in configs/Makefile.$ARCH
dnl

AC_PREREQ(2.60)
AC_INIT([MCSAT],[0])

dnl
dnl Save CFLAGS (AC_PROC_CC modifies it)
dnl
saved_cflags=$CFLAGS

dnl
dnl Build platform
dnl --------------
dnl
AC_CANONICAL_BUILD

AC_SUBST(ARCH)
AC_SUBST(POSIXOS)
POSIXOS=$build_os
ARCH=$build

if test "$POSIXOS" = unknown ; then
   AC_MSG_ERROR([Unknown operating system])
fi


dnl
dnl define SET_MAKE if needed
dnl -------------------------
dnl
AC_PROG_MAKE_SET

dnl
dnl Tools for building MCSAT
dnl ------------------------
dnl
AC_PROG_SED
AC_PROG_LN_S
AC_PROG_MKDIR_P

AC_PROG_CC
AC_PROG_RANLIB
AC_CHECK_PROG(LD,ld,ld)
AC_CHECK_PROG(AR,ar,ar)
AC_CHECK_PROG(STRIP,strip,strip)
AC_PROG_YACC

#
# Restore CFLAGS
#
CFLAGS=$saved_cflags

#
# Store config data in ./configs/make.include.$(ARCH)
#
AC_OUTPUT([make.include])

AC_MSG_NOTICE([Moving make.include to configs/make.include.$build])
mv make.include "configs/make.include.$build"
