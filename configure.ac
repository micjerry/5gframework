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

#######################################
####     Checks for arguments.     ####
#######################################

rundir="${prefix}/run"
logdir="${prefix}/log"
confdir="${prefix}/conf"

default_runtimedir="$rundir"
default_logfiledir="$logdir"
default_certsdir="${prefix}/certs"
default_moddir="${prefix}/mod"


AC_ARG_WITH([rundir],
    [AS_HELP_STRING([--with-rundir=DIR], [Put pidfile into this location (default: $prefix/run)])], [runtimedir="$withval"], [runtimedir="${default_runtimedir}"])
AC_SUBST(runtimedir)
AC_DEFINE_UNQUOTED([AGC_RUN_DIR],"${runtimedir}",[where to put pidfile to])

AC_ARG_WITH([logfiledir],
    [AS_HELP_STRING([--with-logfiledir=DIR], [Put logfiles into this location (default: $localstatedir/log)])], [logfiledir="$withval"], [logfiledir="${default_logfiledir}"])
AC_SUBST(logfiledir)
AC_DEFINE_UNQUOTED([AGC_LOG_DIR],"${logfiledir}",[where to put log files])

AC_ARG_WITH([certsdir],
    [AS_HELP_STRING([--with-certsdir=DIR], [Put certs files into this location (default: $prefix/certs)])], [certsdir="$withval"], [certsdir="${default_certsdir}"])
AC_SUBST(certsdir)
AC_DEFINE_UNQUOTED([AGC_CERTS_DIR],"${certsdir}",[where to put certs files])

AC_ARG_WITH([modinstdir],
    [AS_HELP_STRING([--with-modinstdir=DIR], [Install modules into this location (default: $prefix/mod)])], [modulesdir="$withval"], [modulesdir="${default_modulesdir}"])
eval modulesdir="${modulesdir}"
AC_SUBST(modulesdir)
AC_DEFINE_UNQUOTED([AGC_MOD_DIR],"${modulesdir}",[where to install the modules to])


AC_SUBST(confdir)
AC_DEFINE_UNQUOTED([AGC_CONF_DIR],"${confdir}",[directory for configuration files])

AC_DEFINE_UNQUOTED([AGC_PREFIX_DIR],"${prefix}",[directory for base])

#######################################
####  Checks for gcc and headers.  ####
#######################################

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

TOUCH_TARGET='if test -f "$@";then touch "$@";fi;'

#########################
#### Config modules. ####
#########################

CONF_MODULES='$$(grep -v "\#" $(agc_builddir)/modules.conf | sed -e "s|^.*/||" | sort | uniq )'
CONF_DISABLED_MODULES='$$(grep "\#" $(agc_builddir)/modules.conf | grep -v "\#\#" | sed -e "s|^.*/||" | sort | uniq )'
OUR_MODS='$$(if test -z "$(MODULES)" ; then tmp_mods="$(CONF_MODULES)"; else tmp_mods="$(MODULES)" ; fi ; mods="$$(for i in $$tmp_mods ; do echo $$i-all ; done )"; echo $$mods )'
OUR_CLEAN_MODS='$$(if test -z "$(MODULES)" ; then tmp_mods="$(CONF_MODULES)"; else tmp_mods="$(MODULES)" ; fi ; mods="$$(for i in $$tmp_mods ; do echo $$i-clean ; done )"; echo $$mods )'
OUR_INSTALL_MODS='$$(if test -z "$(MODULES)" ; then tmp_mods="$(CONF_MODULES)"; else tmp_mods="$(MODULES)" ; fi ; mods="$$(for i in $$tmp_mods ; do echo $$i-install ; done)"; echo $$mods )'
OUR_UNINSTALL_MODS='$$(if test -z "$(MODULES)" ; then tmp_mods="$(CONF_MODULES)"; else tmp_mods="$(MODULES)" ; fi ; mods="$$(for i in $$tmp_mods ; do echo $$i-uninstall ; done)"; echo $$mods )'
OUR_DISABLED_MODS='$$(tmp_mods="$(CONF_DISABLED_MODULES)"; mods="$$(for i in $$tmp_mods ; do echo $$i-all ; done )"; echo $$mods )'
OUR_DISABLED_CLEAN_MODS='$$(tmp_mods="$(CONF_DISABLED_MODULES)";  mods="$$(for i in $$tmp_mods ; do echo $$i-clean ; done )"; echo $$mods )'
OUR_DISABLED_INSTALL_MODS='$$(tmp_mods="$(CONF_DISABLED_MODULES)"; mods="$$(for i in $$tmp_mods ; do echo $$i-install ; done)"; echo $$mods )'
OUR_DISABLED_UNINSTALL_MODS='$$(tmp_mods="$(CONF_DISABLED_MODULES)"; mods="$$(for i in $$tmp_mods ; do echo $$i-uninstall ; done)"; echo $$mods )'

#AM_MAKEFLAGS='"OUR_MODULES=$(OUR_MODS)" "OUR_CLEAN_MODULES=$(OUR_CLEAN_MODS)" "OUR_INSTALL_MODULES=$(OUR_INSTALL_MODS)" "OUR_UNINSTALL_MODULES=$(OUR_UNINSTALL_MODS)" "OUR_DISABLED_MODULES=$(OUR_DISABLED_MODS)" "OUR_DISABLED_CLEAN_MODULES=$(OUR_DISABLED_CLEAN_MODS)" "OUR_DISABLED_INSTALL_MODULES=$(OUR_DISABLED_INSTALL_MODS)" "OUR_DISABLED_UNINSTALL_MODULES=$(OUR_DISABLED_UNINSTALL_MODS)" `test -n "$(VERBOSE)" || echo -s`'
#AM_MAKEFLAGS='`test -n "$(VERBOSE)" || echo -s`'

AC_SUBST(CONF_MODULES)
AC_SUBST(CONF_DISABLED_MODULES)
AC_SUBST(OUR_MODS)
AC_SUBST(OUR_CLEAN_MODS)
AC_SUBST(OUR_INSTALL_MODS)
AC_SUBST(OUR_UNINSTALL_MODS)
AC_SUBST(OUR_DISABLED_MODS)
AC_SUBST(OUR_DISABLED_CLEAN_MODS)
AC_SUBST(OUR_DISABLED_INSTALL_MODS)
AC_SUBST(OUR_DISABLED_UNINSTALL_MODS)
AC_SUBST(AM_MAKEFLAGS)

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
AC_CONFIG_FILES([Makefile
        build/Makefile
        src/Makefile
        src/mod/Makefile
        src/mod/loggers/mod_log/Makefile])
AC_OUTPUT

echo 
echo "------------------AGC configuration-------------"
echo "source code location    : ${srcdir}"
echo "compiler                : ${CC}"
echo "build location          : ${agc_builddir}"
echo "modules                 : ${OUR_MODS}"
echo "confmodules             : ${CONF_MODULES}"
echo "agc compiler flags      : ${AGC_AM_CFLAGS}"
echo "yml compile flags       : ${YAML_CFLAGS}"
echo "agc linker flags:       : ${AGC_AM_LDFLAGS}"
echo "yml linker flags:       : ${YAML_LIBS}"
