
include $(top_srcdir)/Makefile.common

SUBDIRS = .

noinst_LTLIBRARIES = libZMapDataSource.la

libZMapDataSource_la_SOURCES = \
zmapDataSlave.cpp \
zmapDataSource.cpp \
zmapDataSourceSlaveHandler.cpp \
zmapDataSource_P.hpp \
zmapOldSlaveHandler.cpp \
zmapOldSourceServer.cpp \
zmapOldSourceServer_P.hpp \
zmapOldStepList.cpp \
$(NULL)

libZMapDataSource_la_CFLAGS = $(INCLUDE_COMMON)


# Extra files to remove for the maintainer-clean target.
#
MAINTAINERCLEANFILES = $(top_srcdir)/zmapDataSource/Makefile.in


