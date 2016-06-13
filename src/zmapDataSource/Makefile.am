
include $(top_srcdir)/Makefile.common

SUBDIRS = .

noinst_LTLIBRARIES = libZMapDataSource.la

# not doing this one any more....
#zmapDataSourceRequest.cpp

libZMapDataSource_la_SOURCES = \
zmapDataSource.cpp \
zmapDataSourceServer.cpp \
zmapDataSourceServer_P.hpp \
zmapDataSourceStepList.cpp \
$(NULL)

libZMapDataSource_la_CFLAGS = $(INCLUDE_COMMON)


# Extra files to remove for the maintainer-clean target.
#
MAINTAINERCLEANFILES = $(top_srcdir)/zmapDataSource/Makefile.in


