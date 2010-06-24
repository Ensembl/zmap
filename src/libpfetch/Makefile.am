
include $(top_srcdir)/Makefile.common

SUBDIRS = . tests

noinst_LTLIBRARIES = libpfetch.la

libpfetch_la_SOURCES = libpfetch-utils.c \
libpfetch.c \
libpfetch.h \
libpfetch_I.h \
libpfetch_P.h \
$(NULL)  

libpfetch_la_CFLAGS = $(INCLUDE_COMMON)

libpfetch_la_marshal = libpfetch-cmarshal.list

libpfetch_la_DEPENDENCIES = $(nodist_libpfetch_la_SOURCES)

libpfetch_la_extra_sources = libpfetch-cmarshal.h \
libpfetch-cmarshal.c \
$(NULL)

nodist_libpfetch_la_SOURCES = $(libpfetch_la_extra_sources)

BUILT_SOURCES = $(libpfetch_la_extra_sources)
CLEANFILES = $(BUILT_SOURCES)
EXTRA_DIST = $(libpfetch_la_marshal)   


GENMARSHAL_COMMAND = $(GLIB_GENMARSHAL) --prefix=libpfetch_cmarshal

.list.h:
	$(GENMARSHAL_COMMAND) --header $< >$@

.list.c:
	$(GENMARSHAL_COMMAND) --body $< >$@

