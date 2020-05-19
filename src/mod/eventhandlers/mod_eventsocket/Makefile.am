include $(top_srcdir)/build/modmake.rulesam

MODNAME=mod_eventsocket
mod_LTLIBRARIES = mod_eventsocket.la
mod_eventsocket_la_SOURCES  = mod_eventsocket.c eventsocket_conn.c rabbitmq_utils.c
mod_eventsocket_la_CFLAGS   = $(AM_CFLAGS) -I.
mod_eventsocket_la_LIBADD   = $(agc_builddir)/libagc.la
mod_eventsocket_la_LDFLAGS  = -avoid-version -module -no-undefined -shared

$(am_mod_eventsocket_la_OBJECTS):  mod_eventsocket.h
