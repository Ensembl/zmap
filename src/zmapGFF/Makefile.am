
include $(top_srcdir)/Makefile.common

SUBDIRS = .

noinst_LTLIBRARIES = libZMapGFF.la

libZMapGFF_la_SOURCES = zmapGFF_P.h \
zmapGFF2Dumper.c \
zmapGFF2parser.c \
$(NULL)

libZMapGFF_la_CFLAGS = $(INCLUDE_COMMON)


# Extra files to remove for the maintainer-clean target.
#
MAINTAINERCLEANFILES = $(top_srcdir)/zmapGFF/Makefile.in



