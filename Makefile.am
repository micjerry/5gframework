## Process this file with automake to produce Makefile.in

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = lib src test

bin_PROGRAMS = agc

nodist_agc_SOURCES = src/include/agc_version.h
agc_SOURCES = src/agc.c
agc_CFLAGS  = $(AM_CFLAGS) $(CORE_CFLAGS)
agc_LDFLAGS = $(AM_LDFLAGS) -lpthread 
agc_LDADD   = libagc.la libs/apr/libapr-1.la

noinst_LTLIBRARIES = libagc.la
libagc_la_SOURCES = \
	src/include/agc.h \
    src/include/agc_am_config.h \
    src/include/agc_apr.h \
    
	pcrf_init.c pcrf_context.c pcrf_fd_path.c pcrf_gx_path.c pcrf_rx_path.c \
	$(NULL)

libagc_la_DEPENDENCIES = \
    $(top_srcdir)/lib/fd/libfd.la \
    $(top_srcdir)/src/common/libcommon.la \
	$(NULL)

libagc_la_LIBADD = \
    $(top_srcdir)/lib/fd/libfd.la \
    $(top_srcdir)/src/common/libcommon.la \
	$(NULL)
   