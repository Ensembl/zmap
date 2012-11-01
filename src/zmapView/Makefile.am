
include $(top_srcdir)/Makefile.common

SUBDIRS = .

noinst_LTLIBRARIES = libZMapView.la

libZMapView_la_SOURCES = zmapView.c \
zmapView_P.h \
zmapViewUtils.c \
zmapViewCallBlixem.c \
zmapViewFeatureMask.c \
zmapViewFeatureCollapse.c \
zmapViewRemoteControl.c \
zmapViewRemoteReceive.c \
zmapViewRemoteSend.c \
zmapViewEditColumn.c \
$(NULL)

libZMapView_la_CFLAGS = $(INCLUDE_COMMON)


# Extra files to remove for the maintainer-clean target.
#
MAINTAINERCLEANFILES = $(top_srcdir)/zmapView/Makefile.in


