
include $(top_srcdir)/Makefile.common

SUBDIRS = .

#AM_CPPFLAGS += -I$(top_srcdir)/zmapServer

noinst_LTLIBRARIES = libDataStream.la

libDataStream_la_SOURCES = \
zmapDataStream.cpp \
zmapDataStream_P.hpp \
$(NULL)

libDataStream_la_LIBADD  = 
libDataStream_la_LDFLAGS =
libDataStream_la_CFLAGS  = $(INCLUDE_COMMON) -I$(top_srcdir)/zmapServer

# Extra files to remove for the maintainer-clean target.
#
MAINTAINERCLEANFILES = $(top_srcdir)/zmapServer/datastream/Makefile.in



