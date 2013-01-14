
include $(top_srcdir)/Makefile.common

SUBDIRS = .

noinst_LTLIBRARIES = libZMapView.la

libZMapView_la_SOURCES = zmapView.c \
zmapViewUtils.c \
zmapViewRemoteReceive.c \
zmapView_P.h \
zmapViewCallBlixem.c \
zmapViewRemoteSend.c \
zmapViewFeatureMask.c \
zmapViewFeatureCollapse.c \
$(NULL)

libZMapView_la_CFLAGS = $(INCLUDE_COMMON)


# Extra files to remove for the maintainer-clean target.
#
MAINTAINERCLEANFILES = $(top_srcdir)/zmapView/Makefile.in


