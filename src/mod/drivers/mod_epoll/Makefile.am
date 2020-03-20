include $(top_srcdir)/build/modmake.rulesam

MODNAME=mod_epoll
mod_LTLIBRARIES = mod_epoll.la
mod_logfile_la_SOURCES  = mod_epoll.c
mod_logfile_la_CFLAGS   = $(AM_CFLAGS)
mod_logfile_la_LIBADD   = $(agc_builddir)/libagc.la
mod_logfile_la_LDFLAGS  = -avoid-version -module -no-undefined -shared
