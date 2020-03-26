include $(top_srcdir)/build/modmake.rulesam
MODNAME=mod_redis

if HAVE_HIREDIS

mod_LTLIBRARIES = mod_redis.la
mod_redis_la_SOURCES  = mod_redis.c
mod_redis_la_CFLAGS   = $(AM_CFLAGS) $(HIREDIS_CFLAGS)
mod_redis_la_LIBADD   = $(agc_builddir)/libagc.la
mod_redis_la_LDFLAGS  = -avoid-version -module -no-undefined -shared $(HIREDIS_LIBS) $(AGC_AM_LDFLAGS)

else
install: error
all: error
error:
	$(error You must install libhiredis-dev to build this module)
endif