
include $(top_srcdir)/Makefile.common

SUBDIRS = .

noinst_LTLIBRARIES = libZMapDraw.la

libZMapDraw_la_SOURCES = zmapDraw_P.h \
zmapDraw.c \
$(NULL)

libZMapDraw_la_CFLAGS = $(INCLUDE_COMMON)


# Extra files to remove for the maintainer-clean target.
#
MAINTAINERCLEANFILES = $(top_srcdir)/zmapDraw/Makefile.in
