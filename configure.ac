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
AM_INIT_AUTOMAKE

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
AC_PREFIX_DEFAULT(/opt/agc)

if test "x$prefix" = "xNONE" ; then
    prefix='/opt/agc'
fi

if test "$includedir" = "\${prefix}/include" ; then
   includedir="${prefix}/include/agc"
fi

rundir="${prefix}/run"
logdir="${prefix}/log"
libdir="${prefix}/lib"

eval full_datadir="${datadir}/agc"
eval datadir=$full_datadir

eval confdir="${sysconfdir}/agc"

default_runtimedir="$rundir"
default_logfiledir="$logdir"
default_certsdir="${prefix}/certs"
default_dbdir="${prefix}/db"

default_modulesdir="${libdir}/agc/mod"

if test "x${exec_prefix}" = "xNONE" ; then
    exec_prefix="$prefix"
fi

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

AC_ARG_WITH([dbdir],
    [AS_HELP_STRING([--with-dbdir=DIR], [Put logfiles into this location (default: $prefix/db)])], [dbdir="$withval"], [dbdir="${default_dbdir}"])
AC_SUBST(dbdir)
AC_DEFINE_UNQUOTED([AGC_DB_DIR],"${dbdir}",[where to put db files])

AC_SUBST(confdir)
AC_DEFINE_UNQUOTED([AGC_CONF_DIR],"${confdir}",[directory for configuration files])

AC_SUBST(datadir)
AC_DEFINE_UNQUOTED([AGC_DATA_DIR],"${datadir}",[directory for data files])

AC_DEFINE_UNQUOTED([AGC_PREFIX_DIR],"${prefix}",[directory for base])

AC_ARG_ENABLE(core-libedit-support,     
  [AS_HELP_STRING([--disable-core-libedit-support], [Compile without libedit Support])])

AS_IF([test "x$enable_core_libedit_support" != "xno"],[
  PKG_CHECK_MODULES([LIBEDIT], [libedit >= 2.11],,[
    AC_MSG_RESULT([no])
    AC_CHECK_LIB([edit], [el_line], [LIBEDIT_LIBS=-ledit])
    AC_CHECK_HEADER([histedit.h], [], [unset LIBEDIT_LIBS])
    AS_IF([test "x$LIBEDIT_LIBS" = "x"], [
      AC_MSG_ERROR([You need to either install libedit-dev (>= 2.11) or configure with --disable-core-libedit-support])
      ])])])
      
AS_IF([test "x$enable_core_libedit_support" != "xno"], [
  # If making changes here, don't forget to run autoheader and
  # update libs/esl/src/include/esl_config_auto.h.in manually.
  AC_DEFINE([HAVE_LIBEDIT], [1], [Define to 1 if you have libedit is available])
  save_LIBS="${LIBS}"
  save_CPPFLAGS="${CPPFLAGS}"
  LIBS="${LIBEDIT_LIBS}"
  CPPFLAGS="${LIBEDIT_CFLAGS}"
  AC_CHECK_DECLS([EL_PROMPT_ESC, EL_REFRESH],,, [[#include <histedit.h>]])
  AC_CHECK_FUNCS([el_wset])
  LIBS="${save_LIBS}"
  CPPFLAGS="${save_CPPFLAGS}"
])

#  Enable 64 bit build
AC_ARG_ENABLE(64,
[AC_HELP_STRING([--enable-64],[build with 64 bit support])],[enable_64="$enable_64"],[enable_64="no"])

#######################################
####           functions           ####
#######################################

path_push_unique () {
  x="$(eval echo \$$1)"
  x="$(path_remove "$x" "$2")"
  if test -z "$x"; then
    eval export $1="$2"
  else
    eval export $1="$2:$x"
  fi
}

path_push_unique PKG_CONFIG_PATH /usr/local/lib/pkgconfig:/usr/local/share/pkgconfig

#######################################
####  Checks for gcc and headers.  ####
#######################################

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

dnl Checks CC 
AC_PROG_MAKE_SET
AC_PROG_MKDIR_P
AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL
AM_PROG_AR

m4_defun([_LT_AC_LANG_F77_CONFIG], [:])
m4_defun([_LT_AC_LANG_GCJ_CONFIG], [:])
m4_defun([_LT_AC_LANG_RC_CONFIG], [:])
AM_PROG_CC_C_O
AC_PROG_LIBTOOL

AX_COMPILER_VENDOR

# Set CC_FOR_BUILD
if test "x${cross_compiling}" = "xyes"; then
    CC_FOR_BUILD=${CC_FOR_BUILD-gcc}
    case "$host" in
    	 arm*-linux-gnueabi*|arm*-*-linux-gnueabi*)
		# spandsp modem
		ac_cv_file__dev_ptmx=yes
		# libjs
		export ac_cv_va_copy=yes
		# srtp
		export ac_cv_file__dev_urandom=yes
		# rpl_malloc
		export ac_cv_func_realloc_0_nonnull=yes
		export ac_cv_func_malloc_0_nonnull=yes
		# apr
		export ac_cv_func_setpgrp_void=yes
		export ac_cv_file__dev_zero=yes
		export apr_cv_tcp_nodelay_with_cork=yes
		export ac_cv_file_dbd_apr_dbd_mysql_c=no
		export ac_cv_sizeof_ssize_t=4
		export apr_cv_mutex_recursive=yes
		export ac_cv_func_pthread_rwlock_init=yes
		export apr_cv_type_rwlock_t=yes
		export apr_cv_process_shared_works=yes
		export apr_cv_mutex_robust_shared=yes
	 ;;
    esac
else
    CC_FOR_BUILD='$(CC)'
fi
AC_SUBST(CC_FOR_BUILD)

#set SOLINK variable based on compiler and host
if test "x${ax_cv_c_compiler_vendor}"  =  "xsun" ; then
    SOLINK="-Bdynamic -dy -G"
elif test "x${ax_cv_c_compiler_vendor}" = "xclang" ; then
    case "$host" in
        *darwin*)
            SOLINK="-dynamic -force-flat-namespace"
        ;;
        *)
            SOLINK="-shared -Xlinker -x"
        ;;

    esac
elif test "x${ax_cv_c_compiler_vendor}" = "xgnu" ; then
    case "$host" in
# older Xcode test for darwin, Xcode 4/5 use clang above
        *darwin*)
            SOLINK="-dynamic -bundle -force-flat-namespace"
        ;;
        *-solaris2*)
            SOLINK="-shared -Xlinker"
        ;;
        *)
            SOLINK="-shared -Xlinker -x"
        ;;
    esac
elif test "x${ax_cv_c_compiler_vendor}" = "xintel" ; then
    case "$host" in
        *)
            SOLINK="-shared -Xlinker -x"
        ;;
    esac
else
    AC_ERROR([Please update configure.in with SOLINK values for your compiler])
fi

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
    netinet/sctp.h \
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

##
# detect libtool major version,
# set libtool library extension based on this
# to work around some brokeness when using 'la' with libtool-1.5
#
AC_MSG_CHECKING([libtool major version])
libtool="${agc_builddir}/libtool"
LIBTOOL_MAJOR_VERSION="`$libtool --version 2>/dev/null| sed -e 's/([[^)]]*)//g;s/^[[^0-9]]*//;s/[[- ]].*//g;q'| awk  'BEGIN { FS = "." } { print $1 }' `"
if test -z "$LIBTOOL_MAJOR_VERSION" ; then
	LIBTOOL_MAJOR_VERSION="`sed -n -e '/^VERSION/{s/^.*=\"\{0,1\}\([[0-9]]\{1,\}\)\..*/\1/;p;}' ${agc_srcdir}/build/ltmain.sh`"
fi
if test -z "$LIBTOOL_MAJOR_VERSION" ; then
	AC_MSG_ERROR([Failed to detect your libtool version])
fi
AC_MSG_RESULT([${LIBTOOL_MAJOR_VERSION}])

LIBTOOL_LIB_EXTEN=so

if test "${LIBTOOL_MAJOR_VERSION}" = "2" ; then
   LIBTOOL_LIB_EXTEN="la"
fi
AC_MSG_RESULT([using libtool library extension... ${LIBTOOL_LIB_EXTEN}])

# Enable debugging (default: on)
AC_ARG_ENABLE(debug,
[AC_HELP_STRING([--disable-debug],[build without debug information])],[enable_debug="$enableval"],[enable_debug="yes"])

if test "${enable_debug}" = "yes"; then
        AC_DEFINE([DEBUG],[],[Enable extra debugging.])
	saved_CFLAGS="$CFLAGS"
	CFLAGS=
        AX_CFLAGS_WARN_ALL_ANSI
	AGC_ANSI_CFLAGS=$CFLAGS
	CFLAGS="$saved_CFLAGS"	

	if test "x${ax_cv_c_compiler_vendor}" = "xgnu" ; then
	    if test "$ac_cv_gcc_supports_w_no_unused_result" = yes; then
	      APR_ADDTO(AGC_AM_CFLAGS, -Wno-unused-result)
	    fi
	    APR_ADDTO(AGC_AM_CFLAGS, -g -ggdb)
	    export DEBUG_CFLAGS="-g -ggdb"
	fi

fi

saved_CFLAGS="$CFLAGS"


# tweak compiler specific flags
if test "x${ax_cv_c_compiler_vendor}"  =  "xsun" ; then
    APR_ADDTO(AGC_AM_CFLAGS, -KPIC)
    APR_ADDTO(AGC_AM_CFLAGS, -DPIC)
    APR_ADDTO(AGC_AM_CFLAGS, -erroff=E_END_OF_LOOP_CODE_NOT_REACHED)
    APR_ADDTO(AGC_AM_CFLAGS, -errtags=yes)
    APR_ADDTO(AGC_AM_CFLAGS, -D__FUNCTION__=__func__ )
    APR_ADDTO(AGC_AM_CFLAGS, -mt)
    APR_ADDTO(AGC_AM_CXXFLAGS, -errtags=yes)
    APR_ADDTO(AGC_AM_CXXFLAGS, -KPIC)
    APR_ADDTO(AGC_AM_CXXFLAGS, -DPIC)
    APR_ADDTO(AGC_AM_CXXFLAGS, "-features=extensions")
    APR_ADDTO(AGC_AM_CXXFLAGS, -D__FUNCTION__=__func__)
    APR_ADDTO(AGC_AM_CXXFLAGS, -mt)

    APR_ADDTO(AGC_AM_LDFLAGS, -R${prefix}/lib)
    if test "${enable_64}" = "yes"; then
        APR_ADDTO(AGC_AM_CFLAGS, -m64)
        APR_ADDTO(AGC_AM_CXXFLAGS, -m64)
        APR_ADDTO(AGC_AM_LDFLAGS, -m64)
	LIBS="$LIBS -m64"
    fi
elif test "x${ax_cv_c_compiler_vendor}" = "xclang" ; then
    APR_ADDTO(AGC_AM_CFLAGS, -fPIC)
    APR_ADDTO(AGC_AM_CXXFLAGS, -fPIC)
    APR_ADDTO(AGC_AM_CFLAGS, -Werror)
elif test "x${ax_cv_c_compiler_vendor}" = "xgnu" ; then
    APR_ADDTO(AGC_AM_CFLAGS, -fPIC)
    APR_ADDTO(AGC_AM_CXXFLAGS, -fPIC)
    if test "$ac_cv_gcc_supports_w_no_unused_result" = yes; then
      APR_ADDTO(AGC_AM_CFLAGS, -Werror)
    fi
    if test "${enable_64}" = "yes"; then
	case "$host" in
        *darwin*)
            osxvrm=`sw_vers -productVersion`          # Get version.release.modlevel
	        osxrel=`echo $osxvrm | cut -d. -f2`       # Get release only
            if test "$osxrel" -ge 4; then             # 10.4 and up are x64
		        APR_ADDTO(CFLAGS, -arch x86_64)
		        APR_ADDTO(LDFLAGS, -arch x86_64)
		        APR_ADDTO(CXXFLAGS, -arch x86_64)
            fi
		    ;;
        *-solaris2*)
		    APR_ADDTO(CFLAGS, -m64)
		    APR_ADDTO(LDFLAGS, -m64)
		    APR_ADDTO(CXXFLAGS, -m64)
	        ;;
	     *)
	     	LIBS="$LIBS -m64"
	     ;;
	 esac
    fi
fi

AC_CACHE_CHECK([whether compiler supports -Wdeclaration-after-statement], [ac_cv_gcc_declaration_after_statement], [
CFLAGS="$CFLAGS -Wdeclaration-after-statement"
AC_TRY_COMPILE([],[return 0;],[ac_cv_gcc_declaration_after_statement=yes],[ac_cv_gcc_declaration_after_statement=no])
])
AC_MSG_RESULT($ac_cv_gcc_declaration_after_statement)

if test x"$ac_cv_gcc_declaration_after_statement" = xyes; then
    APR_ADDTO(AGC_ANSI_CFLAGS, -Wdeclaration-after-statement)
fi
CFLAGS="$saved_CFLAGS"

#Check for c++ 11
AX_CXX_COMPILE_STDCXX([11], [noext], [mandatory])
AS_IF([test "x$HAVE_CXX11" != "x1"],
      [AC_MSG_ERROR([** A compiler with C++11 language features is required.])])
	  
APR_ADDTO(AGC_AM_CXXFLAGS, -std=c++0x)

AC_SUBST(AGC_AM_CFLAGS)
AC_SUBST(AGC_ANSI_CFLAGS)
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
PKG_CHECK_MODULES([AMQP], [librabbitmq >= 0.5.2],[
  AM_CONDITIONAL([HAVE_AMQP],[true])],[
  AC_MSG_RESULT([no]); AM_CONDITIONAL([HAVE_AMQP],[false])])
PKG_CHECK_MODULES([SQLITE], [sqlite3 >= 3.6.20])
PKG_CHECK_MODULES([HIREDIS_VIP], [hiredis_vip >= 1.0.0],[
  AM_CONDITIONAL([HAVE_HIREDIS_VIP],[true])],[
  AC_MSG_RESULT([no]); AM_CONDITIONAL([HAVE_HIREDIS_VIP],[false])])

PKG_CHECK_MODULES([NGAPCODEC], [ngapcodec-15.3.0 >= 15.3.0])

AC_SUBST(YAML_CFLAGS)
AC_SUBST(YAML_LIBS)

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
AC_SUBST(LIBTOOL_LIB_EXTEN)

LIBTOOL='$(SHELL) $(agc_builddir)/libtool'
AC_SUBST(LIBTOOL)

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

AC_CONFIG_HEADER([libs/esl/src/include/esl_config_auto.h])

AC_CONFIG_FILES([src/include/agc_version.h])
AC_CONFIG_FILES([src/include/agc_am_config.h])
AC_CONFIG_FILES([Makefile
        build/Makefile
        build/modmake.rules
        src/Makefile
        src/mod/Makefile
        src/mod/loggers/mod_logfile/Makefile
        src/mod/drivers/mod_epoll/Makefile
        src/mod/caches/mod_redis/Makefile
        src/mod/dbs/mod_sqlite/Makefile
        src/mod/mqs/mod_rabbitmq/Makefile
        src/mod/eventhandlers/mod_eventsocket/Makefile
        src/mod/tests/mod_test/Makefile
        src/mod/gateways/mod_sig/Makefile])
AC_OUTPUT

echo 
echo "------------------AGC configuration-------------"
echo "source code location    : ${srcdir}"
echo "compiler                : ${CC}"
echo "build location          : ${agc_builddir}"
echo "modules                 : ${OUR_MODS}"
echo "confmodules             : ${CONF_MODULES}"
echo "agc compiler flags      : ${AGC_AM_CFLAGS}"
echo "c++ compiler flags      : ${AGC_AM_CXXFLAGS}"
echo "agc linker flags:       : ${AGC_AM_LDFLAGS}"
echo "agc cpp flags:          : ${CPPFLAGS}"
echo "agc cxx flags:          : ${CXXFLAGS}"
echo "agc amcpp flags:        : ${AGC_AM_CPPFLAGS}"
echo "yml compile flags       : ${YAML_CFLAGS}"
echo "yml linker flags:       : ${YAML_LIBS}"
