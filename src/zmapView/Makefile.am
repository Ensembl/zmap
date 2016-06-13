
include $(top_srcdir)/Makefile.common

SUBDIRS = .

noinst_LTLIBRARIES = libZMapView.la

libZMapView_la_SOURCES = \
zmapView.cpp \
zmapViewCallBlixem.cpp \
zmapViewCommand.cpp \
zmapViewFeatureMask.cpp \
zmapViewFeatureCollapse.cpp \
zmapViewRemoteControl.cpp \
zmapViewScratch.cpp \
zmapViewServers.cpp \
zmapViewUtils.cpp \
zmapView_P.hpp \
$(NULL)

libZMapView_la_CFLAGS = $(INCLUDE_COMMON)


# Extra files to remove for the maintainer-clean target.
#
MAINTAINERCLEANFILES = $(top_srcdir)/zmapView/Makefile.in


