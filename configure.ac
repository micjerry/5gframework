dnl Process this file with autoconf to produce a configure script.
dnl
dnl This file is free software; as a special exception the author gives
dnl unlimited permission to copy and/or distribute it, with or without
dnl modifications, as long as this notice is preserved.
dnl
dnl This program is distributed in the hope that it will be useful, but
dnl WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
dnl implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

AC_INIT([AGC], [0.1.0], [help@agc.com])

AC_SUBST(AGC_VERSION_MAJOR, [0])
AC_SUBST(AGC_VERSION_MINOR, [1])
AC_SUBST(AGC_VERSION_MICRO, [0])

AC_SUBST(LIBVERSION)
LIBVERSION=1:0:0

dnl Must come before AM_INIT_AUTOMAKE.
AC_CONFIG_AUX_DIR([build])
AC_CONFIG_TESTDIR(test)
AM_INIT_AUTOMAKE([1.10 -Wall -Werror foreign subdir-objects])

AC_CONFIG_HEADERS([config.h:config.in])
AC_CONFIG_SRCDIR([src/agc.c])
AC_CANONICAL_HOST

case $host in
    *linux*)
        OSDIR="unix"
        OSCPPFLAGS="-DLINUX=1"
        ;;
    *-apple-darwin*)
        OSDIR="unix"
        OSCPPFLAGS="-DDARWIN -DSIGPROCMASK_SETS_THREAD_MASK"
        ;;
    *)
        OSDIR="unix"
        ;;    
esac

AC_SUBST(OSCPPFLAGS)
AC_SUBST(OSDIR)

AH_TOP([
#ifndef __AGC_CONFIG_H__
#define __AGC_CONFIG_H__
/* need this, because some autoconf tests rely on this (e.g. stpcpy)
 * and it should be used for new programs  */
#define _DEFAULT_SOURCE 1
#define _BSD_SOURCE  	1
])

AH_BOTTOM([
#endif /* __AGC_CONFIG_H__ */
])

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

dnl Checks CC 
AC_PROG_MAKE_SET
AC_PROG_MKDIR_P
AC_PROG_CC
AC_PROG_INSTALL
AM_PROG_AR

LT_INIT([pic-only disable-static])

AC_PATH_PROG(PKG_CONFIG, pkg-config, no)
if test "x$PKG_CONFIG" = "xno"; then
	AC_MSG_ERROR([You need to install pkg-config])
fi

PKG_PROG_PKG_CONFIG([0.20])

AC_HEADER_STDC

AC_CHECK_HEADERS( \
    sys/stat.h \
    sys/types.h \
    pthread.h \
    time.h \
    signal.h \
    stdio.h \
    stddef.h \
    assert.h \
    setjmp.h \
    ctype.h \
    fcntl.h \
    string.h \
    strings.h \
    stdio.h \
    stdint.h \
    stdlib.h \
    string.h \
    strings.h \
    limits.h \
    errno.h \
    inttypes.h \
    arpa/inet.h \
    sys/socket.h \
    netinet/in.h \
    netdb.h \
)

##########################################
####    Checks for compile flags.     ####
##########################################
agc_builddir=`pwd`
agc_srcdir=`(cd $srcdir && pwd)`
AC_SUBST(agc_builddir)
AC_SUBST(agc_srcdir)

# set defaults for use on all platforms
AGC_AM_CFLAGS="-I${agc_srcdir}/src/include -I${agc_builddir}/src/include"
AGC_AM_CXXFLAGS="-I${agc_srcdir}/src/include -I${agc_builddir}/src/include"
AGC_AM_CPPFLAGS="-I${agc_srcdir}/src/include -I${agc_builddir}/src/include"
AGC_AM_LDFLAGS="-lm"

AC_SUBST(AGC_AM_CFLAGS)
AC_SUBST(AGC_AM_CXXFLAGS)
AC_SUBST(AGC_AM_CPPFLAGS)
AC_SUBST(AGC_AM_LDFLAGS)

##########################################
#### Checks for typedefs, structures, ####
####  and compiler characteristics.   ####
##########################################

AC_C_BIGENDIAN

AC_CHECK_FILE(/dev/null,
    AC_DEFINE([HAVE_DEV_NULL], [1],
        [Define to 1 if you have the /dev/null file.]))
        
#######################################
#### Checks for library functions. ####
#######################################

AC_FUNC_VPRINTF
AC_CHECK_FUNCS(\
    gettimeofday \
    strerror \
    sigaction sigwait sigsuspend \
    strcpy strcasecmp strtoul stricmp \
    writev \
    utime utimes \
    sched_yield \
    getenv putenv setenv unsetenv \
)

AC_SEARCH_LIBS([sctp_sendmsg], [sctp], [have_sctp_lib=yes], [have_sctp_lib=no])
if test x$have_sctp_lib == xno; then
  AC_SEARCH_LIBS([usrsctp_init], [usrsctp], [have_usrsctp_lib=yes], [have_usrsctp_lib=no])
  if test x$have_usrsctp_lib == xno; then
    AC_MSG_ERROR([You must install the SCTP libraries and development headers to enable SCTP support.])
  else
    AC_DEFINE([USE_USRSCTP], [1], [Define to 1 if you have the user sctp library.])
  fi 
fi

PKG_CHECK_MODULES([YAML], yaml-0.1 >= 0.1.4)

AC_SUBST(YAML_CFLAGS)
AC_SUBST(YAML_LIBS)

if test "x$prefix" = "xNONE" ; then
    prefix='/opt/agc'
fi

if test "$includedir" = "\${prefix}/include" ; then
   includedir="${prefix}/include/agc"
fi

AC_SUBST(includedir)

AC_CHECK_LIB(apr-1, apr_pool_mutex_set, use_system_apr=yes, use_system_apr=no)
AM_CONDITIONAL([SYSTEM_APR],[test "${use_system_apr}" = "yes"])
AC_CHECK_LIB(aprutil-1, apr_queue_pop_timeout, use_system_aprutil=yes, use_system_aprutil=no)
AM_CONDITIONAL([SYSTEM_APRUTIL],[test "${use_system_aprutil}" = "yes"])

AC_SUBST(SYSTEM_APR)
AC_SUBST(SYSTEM_APRUTIL)

#####################
#### Conclusion. ####
#####################

ac_configure_args="$ac_configure_args CONFIGURE_CFLAGS='$CFLAGS $CPPFLAGS' CONFIGURE_CXXFLAGS='$CXXFLAGS $CPPFLAGS' CONFIGURE_LDFLAGS='$LDFLAGS' "

if test "$use_system_apr" != "yes"; then
   AC_CONFIG_SUBDIRS([libs/apr])
fi
if test "$use_system_aprutil" != "yes"; then
   ac_configure_args="$ac_configure_args --with-apr=../apr" 
   AC_CONFIG_SUBDIRS([libs/apr-util])
fi

AC_CONFIG_FILES([src/include/agc_version.h])
AC_CONFIG_FILES([src/include/agc_am_config.h])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT

echo 
echo "------------------AGC configuration-------------"
echo "source code location    : ${srcdir}"
echo "compiler                : ${CC}"
echo "agc compiler flags      : ${AGC_AM_CFLAGS}"
echo "yml compile flags       : ${YAML_CFLAGS}"
echo "agc linker flags:       : ${AGC_AM_LDFLAGS}"
echo "yml linker flags:       : ${YAML_LIBS}"
