include $(top_srcdir)/build/modmake.rulesam
MODNAME=mod_test

mod_LTLIBRARIES = mod_test.la
mod_test_la_SOURCES  = mod_test.c test_cache.c test_api.c test_event.c test_mq.c test_driver.c
mod_test_la_CFLAGS   = $(AM_CFLAGS)
mod_test_la_LIBADD   = $(agc_builddir)/libagc.la
mod_test_la_LDFLAGS  = -avoid-version -module -no-undefined -shared $(AGC_AM_LDFLAGS)

$(am_mod_test_la_OBJECTS):  mod_test.h
