## Process this file with automake to produce Makefile.in

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = . src build
AUTOMAKE_OPTIONS = foreign subdir-objects

all-recursive: libagc.la

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

AM_CFLAGS   = $(AGC_AM_CFLAGS) $(YAML_CFLAGS)
AM_CPPFLAGS = $(AGC_AM_CXXFLAGS)
AM_LDFLAGS  = $(AGC_AM_LDFLAGS) $(AM_LIBAPR_LDFLAGS) $(AM_LIBAPU_LDFLAGS)

CLEANFILES    = src/include/agc_version.h src/include/agc_am_config.h

##
## libagc
##
lib_LTLIBRARIES = libagc.la

library_includedir      = $(includedir)
library_include_HEADERS = \
	src/include/agc.h \
	src/include/agc_apr.h \
	src/include/agc_core.h \
	src/include/agc_event.h \
	src/include/agc_log.h \
	src/include/agc_platform.h \
	src/include/agc_types.h \
	src/include/agc_utils.h \
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
	src/agc_time.c \
	src/agc_utils.c \
	$(NULL)

libagc_la_CFLAGS  = $(CORE_CFLAGS)
libagc_la_LDFLAGS = -version-info 1:0:0 $(AM_LDFLAGS) -no-undefined
libagc_la_LIBADD = $(CORE_LIBS) $(APR_LIBS) $(YAML_LIBS)

##
## Applications
##
bin_PROGRAMS = agc

nodist_agc_SOURCES = src/include/agc_version.h
agc_SOURCES = src/agc.c
agc_CFLAGS  = $(AM_CFLAGS) $(CORE_CFLAGS)
agc_LDFLAGS = $(AM_LDFLAGS) -lpthread 
agc_LDADD   = libagc.la libs/apr/libapr-1.la

##
## Dependency targets
##
libs/apr/Makefile: libs/apr/Makefile.in libs/apr/config.status libs/apr
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


MAINTAINERCLEANFILES = Makefile.in
MOSTLYCLEANFILES = core *.stackdump
