## Process this file with automake to produce Makefile.in

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = . src build
AUTOMAKE_OPTIONS = foreign subdir-objects
NAME = agc

all-recursive: libagc.la
clean-recusive: clean_core
install-recursive: install-libLTLIBRARIES install-binPROGRAMS

if SYSTEM_APR
AM_LIBAPR_CFLAGS := $(shell apr-1-config --cflags)
AM_LIBAPR_CPPFLAGS := $(shell apr-1-config --cppflags --includes)
AM_LIBAPR_LDFLAGS := $(shell apr-1-config --ldflags)
AM_LIBAPR_LIBS := $(shell apr-1-config \--libs)
AM_LIBAPR_LINKLIBTOOL := $(shell apr-1-config \--link-libtool)
else
AM_LIBAPR_CFLAGS := $(shell ./libs/apr/apr-1-config --cflags)
AM_LIBAPR_CPPFLAGS := $(shell ./libs/apr/apr-1-config --cppflags --includes)
AM_LIBAPR_LDFLAGS := $(shell ./libs/apr/apr-1-config --ldflags)
AM_LIBAPR_LIBS := $(subst $(agc_builddir)/,,$(shell ./libs/apr/apr-1-config \--libs))
endif
if SYSTEM_APRUTIL
AM_LIBAPU_CPPFLAGS := $(shell apu-1-config --includes)
AM_LIBAPU_LDFLAGS := $(shell apu-1-config --ldflags)
AM_LIBAPU_LIBS := $(shell apu-1-config \--libs)
AM_LIBAPU_LINKLIBTOOL := $(shell apu-1-config \--link-libtool)
else
AM_LIBAPU_CPPFLAGS := $(shell ./libs/apr-util/apu-1-config --includes)
AM_LIBAPU_LDFLAGS := $(shell ./libs/apr-util/apu-1-config --ldflags)
AM_LIBAPU_LIBS := $(subst $(agc_builddir)/,,$(shell ./libs/apr-util/apu-1-config \--libs))
endif

APR_LIBS   = $(AM_LIBAPU_LIBS) $(AM_LIBAPR_LIBS)

CORE_LIBS=
if SYSTEM_APRUTIL
CORE_LIBS += $(AM_LIBAPU_LINKLIBTOOL)
else
CORE_LIBS +=  libs/apr-util/libaprutil-1.la
endif
if SYSTEM_APR
CORE_LIBS += $(AM_LIBAPR_LINKLIBTOOL)
else
CORE_LIBS += libs/apr/libapr-1.la
endif

CORE_CFLAGS  = $(AM_LIBAPR_CFLAGS) $(AM_LIBAPR_CPPFLAGS)
CORE_CFLAGS += $(AM_LIBAPU_CPPFLAGS)

CORE_CFLAGS += $(ODBC_INC_FLAGS)

AM_CFLAGS   = $(AGC_AM_CFLAGS) $(YAML_CFLAGS) $(SQLITE_CFLAGS)
AM_CXXFLAGS = $(AGC_AM_CXXFLAGS)
AM_CPPFLAGS = $(AGC_AM_CPPFLAGS)
AM_LDFLAGS  = $(AGC_AM_LDFLAGS) $(AM_LIBAPR_LDFLAGS) $(AM_LIBAPU_LDFLAGS)

##
## libagc
##

BUILT_SOURCES = src/mod/modules.inc src/include/agc_version.h

lib_LTLIBRARIES = libagc.la

library_includedir      = $(includedir)
library_include_HEADERS = \
	src/include/agc.h \
	src/include/agc_apr.h \
	src/include/agc_core.h \
	src/include/agc_log.h \
	src/include/agc_platform.h \
	src/include/agc_types.h \
	src/include/agc_utils.h \
	src/include/agc_memory.h \
	src/include/agc_module.h \
	src/include/agc_dso.h \
	src/include/agc_mprintf.h \
	src/include/agc_json.h \
	src/include/agc_rbtree.h \
	src/include/agc_timer.h \
	src/include/agc_event.h \
	src/include/agc_driver.h \
	src/include/agc_connection.h \
	src/include/agc_api.h \
	src/include/agc_cache.h \
	src/include/agc_db.h \
	src/include/agc_sctp.h \
	src/include/agc_odbc.h \
	src/include/agc_sql.h \
	src/include/private/agc_core_pvt.h
 
nodist_libagc_la_SOURCES = \
	src/include/agc_version.h \
	src/include/agc_am_config.h

libagc_la_SOURCES = \
	src/agc_apr.c \
	src/agc_config.c \
	src/agc_core.c \
	src/agc_core_memory.c \
	src/agc_log.c \
	src/agc_rbtree.c \
	src/agc_timer.c \
	src/agc_utils.c \
	src/agc_module.c \
	src/agc_dso.c \
	src/agc_mprintf.c \
	src/agc_json.c \
	src/agc_event.c \
	src/agc_connection.c \
	src/agc_driver.c \
	src/agc_api.c \
	src/agc_cache.c \
	src/agc_db.c \
	src/agc_sctp.c \
	src/agc_odbc.c \
	src/agc_sql.c \
	$(NULL)

$(libagc_la_SOURCES): $(CORE_LIBS) $(agc_builddir)/modules.conf  

libagc_la_CFLAGS  = $(CORE_CFLAGS)
libagc_la_LDFLAGS = -version-info 1:0:0 $(AM_LDFLAGS) -no-undefined
libagc_la_LIBADD = $(CORE_LIBS) $(APR_LIBS) $(YAML_LIBS) $(SQLITE_LIBS)
libagc_la_DEPENDENCIES = $(BUILT_SOURCES)

libagc_la_LDFLAGS += $(ODBC_LIB_FLAGS)

##
## Applications
##
bin_PROGRAMS = agc agc_cli


##
## agc_cli
##
agc_cli_SOURCES = libs/esl/src/esl.c libs/esl/src/esl_config.c libs/esl/src/esl_event.c \
	libs/esl/src/esl_threadmutex.c libs/esl/agc_cli.c libs/esl/src/esl_json.c libs/esl/src/esl_buffer.c
agc_cli_CFLAGS  = $(AM_CFLAGS) -I$(agc_srcdir)/libs/esl/src/include $(LIBEDIT_CFLAGS)
agc_cli_LDFLAGS = $(AM_LDFLAGS) -lpthread -lm $(LIBEDIT_LIBS)

##
## agc
##
nodist_agc_SOURCES = src/include/agc_version.h
agc_SOURCES = src/agc.c
agc_CFLAGS  = $(AM_CFLAGS) $(CORE_CFLAGS)
agc_LDFLAGS = $(AM_LDFLAGS) -lpthread 
agc_LDADD   = libagc.la libs/apr/libapr-1.la

agc_LDADD += $(ODBC_LIB_FLAGS)

##
## Dependency targets
##
libs/apr/Makefile: libs/apr/Makefile.in libs/apr/config.status
	@cd libs/apr && ./config.status
	@$(TOUCH_TARGET)

libs/apr/libapr-1.la: libs/apr/Makefile
	@if [ $(MAKELEVEL) = 0 -o -z "`echo "$(MAKEARGS)" | grep  "j"`" ] ; then touch $(agc_srcdir)/src/include/agc.h; cd libs/apr && $(MAKE) $(MFLAGS) && touch libapr-1.la; fi

libs/apr-util/libaprutil-1.la: libs/apr/libapr-1.la libs/apr-util
	@if [ $(MAKELEVEL) = 0 -o -z "`echo "$(MAKEARGS)" | grep  "j"`" ] ; then touch $(agc_srcdir)/src/include/agc.h; cd libs/apr-util && $(MAKE) $(MFLAGS) && touch libaprutil-1.la; fi

##
## Modules
##

src/mod/modules.inc: $(agc_builddir)/modules.conf
	@echo "OUR_MODULES=$(OUR_MODS)" > $(agc_builddir)/src/mod/modules.inc
	@echo "OUR_CLEAN_MODULES=$(OUR_CLEAN_MODS)" >> $(agc_builddir)/src/mod/modules.inc
	@echo "OUR_INSTALL_MODULES=$(OUR_INSTALL_MODS)" >> $(agc_builddir)/src/mod/modules.inc
	@echo "OUR_UNINSTALL_MODULES=$(OUR_UNINSTALL_MODS)" >> $(agc_builddir)/src/mod/modules.inc
	@echo "OUR_DISABLED_MODULES=$(OUR_DISABLED_MODS)" >> $(agc_builddir)/src/mod/modules.inc
	@echo "OUR_DISABLED_CLEAN_MODULES=$(OUR_DISABLED_CLEAN_MODS)" >> $(agc_builddir)/src/mod/modules.inc
	@echo "OUR_DISABLED_INSTALL_MODULES=$(OUR_DISABLED_INSTALL_MODS)" >> $(agc_builddir)/src/mod/modules.inc
	@echo "OUR_DISABLED_UNINSTALL_MODULES=$(OUR_DISABLED_UNINSTALL_MODS)" >> $(agc_builddir)/src/mod/modules.inc

$(OUR_MODULES): $(agc_builddir)/modules.conf liagc.la src/mod/modules.inc
	@set fnord $$MAKEFLAGS; amf=$$2; \
	(cd src/mod && $(MAKE) $(AM_MAKEFLAGS) $@) \
	   || case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac; \
	test -z "$$fail"

modules: libagc.la $(agc_builddir)/modules.conf src/mod/modules.inc
	@cd src/mod && $(MAKE) $(AM_MAKEFLAGS)

install_mod: libagc.la $(agc_builddir)/modules.conf src/mod/modules.inc
	@cd src/mod && $(MAKE) $(AM_MAKEFLAGS) install

mod_install: install_mod

uninstall_mod: $(agc_builddir)/modules.conf src/mod/modules.inc
	@cd src/mod && $(MAKE) $(AM_MAKEFLAGS) uninstall

mod_uninstall: uninstall_mod

config-%:
	test -d $(DESTDIR)$(confdir) || $(mkinstalldirs) $(DESTDIR)$(confdir)
	for conffile in `cd $(agc_srcdir)/conf/$* && find . -name \*.yml && find . -name \*.conf` ; do \
		dir=`echo $$conffile | sed -e 's|^\.||' | sed -e 's|/[^/]*$$||'`; \
		filename=`echo $$conffile | sed -e 's|^\.||' | sed -e 's|^.*/||'`; \
		test -d $(DESTDIR)$(confdir)$$dir || $(mkinstalldirs) $(DESTDIR)$(confdir)$$dir ; \
		test -f $(DESTDIR)$(confdir)$$dir/$$filename || \
		test -f $(DESTDIR)$(confdir)$$dir/$$filename.noload || \
			$(INSTALL) -m 644 $(agc_srcdir)/conf/$*/$$dir/$$filename $(DESTDIR)$(confdir)$$dir; \
	done

samples-conf: config-default

samples: samples-conf


db-conf:
	test -d $(DESTDIR)$(dbdir) || $(mkinstalldirs) $(DESTDIR)$(dbdir)
	for dbfile in `cd $(agc_srcdir)/db/$* && find . -name \*.db` ; do \
	    filename=`echo $$dbfile | sed -e 's|^\.||' | sed -e 's|^.*/||'`; \
		test -f $(DESTDIR)$(dbdir)/$$filename || \
		$(INSTALL) -m 644 $(agc_srcdir)/db/$$filename $(DESTDIR)$(dbdir); \
	done

install-exec-local:
	$(mkinstalldirs) $(DESTDIR)$(modulesdir)

install-data-local:
	@echo Installing $(NAME)
	@for x in $(modulesdir) $(runtimedir) $(logfiledir); do \
		$(mkinstalldirs) $(DESTDIR)$$x ; \
	done
	test -d $(DESTDIR)$(confdir) || $(MAKE) samples-conf
	test -d $(DESTDIR)$(dbdir) || $(MAKE) db-conf

##
## Helper targets
##

core: $(agc_builddir)/modules.conf src/include/agc_version.h $(CORE_LIBS) 
	$(MAKE) $(AM_MAKEFLAGS) libagc.la

core-install: core_install
core_install: install-libLTLIBRARIES

clean_core: clean-libLTLIBRARIES
	rm -f $(libagc_la_OBJECTS)
	rm -f `echo $(libagc_la_OBJECTS) | sed -e's|.lo|.o|g'`

.DEFAULT: $(agc_builddir)/modules.conf src/mod/modules.inc
    @target=`echo $@ | sed -e 's|^.*-||'`; \
    target_prefix=`echo $@ | sed -e 's|-.*$$||'`; \
    args="$@"; \
    if test -z "$$args" || test "$${args#-l*}" = "$$args"; then \
        if test "$$target" = "install"; then \
            $(MAKE) $(AM_MAKEFLAGS) core_install && cd src/mod && $(MAKE) $(AM_MAKEFLAGS) $@ ; \
        else \
            if test "$$target" = "clean"; then \
                cd src/mod && $(MAKE) $(AM_MAKEFLAGS) $@ ;\
            else \
                $(MAKE) $(AM_MAKEFLAGS) core && cd src/mod && $(MAKE) $(AM_MAKEFLAGS) $@ ;\
            fi; \
        fi; \
    fi

MAINTAINERCLEANFILES = Makefile.in
MOSTLYCLEANFILES = core *.stackdump
