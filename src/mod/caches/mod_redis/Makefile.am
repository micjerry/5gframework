include $(top_srcdir)/build/modmake.rulesam
MODNAME=mod_redis

if HAVE_HIREDIS_VIP

mod_LTLIBRARIES = mod_redis.la
mod_redis_la_SOURCES  = mod_redis.c
mod_redis_la_CFLAGS   = $(AM_CFLAGS) $(HIREDIS_VIP_CFLAGS)
mod_redis_la_LIBADD   = $(agc_builddir)/libagc.la
mod_redis_la_LDFLAGS  = -avoid-version -module -no-undefined -shared $(HIREDIS_VIP_LIBS) $(AGC_AM_LDFLAGS)

else
install: error
all: error
error:
	$(error You must install hiredis-vip to build this module)
endif