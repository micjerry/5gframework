include $(top_srcdir)/build/modmake.rulesam

MODNAME=mod_epoll
mod_LTLIBRARIES = mod_epoll.la
mod_epoll_la_SOURCES  = mod_epoll.c
mod_epoll_la_CFLAGS   = $(AM_CFLAGS)
mod_epoll_la_LIBADD   = $(agc_builddir)/libagc.la
mod_epoll_la_LDFLAGS  = -avoid-version -module -no-undefined -shared
