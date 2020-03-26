include $(top_srcdir)/build/modmake.rulesam
MODNAME=mod_sqlite

mod_LTLIBRARIES = mod_sqlite.la
mod_sqlite_la_SOURCES  = mod_sqlite.c
mod_sqlite_la_CFLAGS   = $(AM_CFLAGS) $(SQLITE_CFLAGS)
mod_sqlite_la_LIBADD   = $(agc_builddir)/libagc.la
mod_sqlite_la_LDFLAGS  = -avoid-version -module -no-undefined -shared $(SQLITE_LIBS)