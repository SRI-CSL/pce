dnl
dnl Configure for MCSAT, XPCE, and PCE
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

dnl
dnl Need xmlrpc-c for xpce
dnl --------------------------------------------
AC_CHECK_PROG(XMLRPCCCONFIG,xmlrpc-c-config,xmlrpc-c-config)
if test "x$XMLRPCCCONFIG" != x; then
   XMLRPC_SERVER_LIBS=`xmlrpc-c-config abyss-server --libs`
   XMLRPC_CLIENT_LIBS=`xmlrpc-c-config client --libs`
   echo "XMLRPC_SERVER_LIBS = ${XMLRPC_SERVER_LIBS}"
   echo "XMLRPC_CLIENT_LIBS = ${XMLRPC_CLIENT_LIBS}"
fi
   

dnl
dnl OAA Libraries (optional, but needed for PCE)
dnl --------------------------------------------
liboaa=""
AC_ARG_WITH([oaa],
   [AS_HELP_STRING([--with-oaa=<path>],[Full path to OAA])],
   [if test "x$withval" == x; then
      AC_MSG_WARN([--with-oaa was used but no path was given.])
    else
      liboaa=$withval
    fi
   ])

if test "x$liboaa" == x; then
    AC_MSG_WARN([OAA not provided, pce will not be built (use --with-oaa if desired).])
  else
    if test -f $liboaa/src/oaalib/c/include/liboaa.h; then
      if test $build == x86_64-unknown-linux-gnu; then
        oaalibdir=$liboaa/lib/x86-linux
      else if test $build == i686-pc-linux-gnu; then
        oaalibdir=$liboaa/lib/x86-linux
      else if test $build == i386-apple-darwin9-3.0; then
        oaalibdir=$liboaa/lib/mac-os-x
      else if test $build == sparc-sun-solaris2.9; then
        oaalibdir=$liboaa/lib/sparc-solaris
      else if test $build == i686-pc-cygwin; then
        oaalibdir=$liboaa/lib/x86-linux
	LIBS="$LIBS -lintl -liconv  -lglib-2.0"
      else AC_MSG_WARN([$build platform not supported])
      fi fi fi fi fi
      if test ! -f $oaalibdir/liboaa2.a; then
        AC_MSG_WARN([$aalibdir/liboaa2.a not found])
      fi
      OAACFLAGS="-I$liboaa/src/oaalib/c/include"
      OAALDFLAGS="-L$oaalibdir -loaa2"
      LIBS="$LIBS -loaa2"
   else
      AC_MSG_ERROR([*** liboaa.h not found in $liboaa (should be in $liboaa/src/oaalib/c/include/). Check --with-oaa option ***])
   fi
fi

AC_SUBST(XMLRPC_SERVER_LIBS)
AC_SUBST(XMLRPC_CLIENT_LIBS)
AC_SUBST(OAACFLAGS)
AC_SUBST(OAALDFLAGS)

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
