include $(top_srcdir)/build/modmake.rulesam
MODNAME=mod_rabbitmq

if HAVE_AMQP

mod_LTLIBRARIES = mod_rabbitmq.la
mod_rabbitmq_la_SOURCES  = mod_rabbitmq.c
mod_rabbitmq_la_CFLAGS   = $(AM_CFLAGS) $(AMQP_CFLAGS)
mod_rabbitmq_la_LIBADD   = $(agc_builddir)/libagc.la
mod_rabbitmq_la_LDFLAGS  = -avoid-version -module -no-undefined -shared $(AMQP_LIBS) $(AGC_AM_LDFLAGS)

else
install: error
all: error
error:
        $(error You must install librabbitmq1 and librabbitmq-dev to build this module)
endif
