
include $(top_srcdir)/Makefile.common

SUBDIRS = .

noinst_LTLIBRARIES = libZMapAppServices.la

libZMapAppServices_la_SOURCES = \
zmapAppServices_P.hpp \
zmapAppDebugConfig.cpp \
zmapAppSequenceView.cpp \
zmapAppSource.cpp \
$(NULL)

libZMapAppServices_la_CFLAGS = $(INCLUDE_COMMON)


# Extra files to remove for the maintainer-clean target.
#
MAINTAINERCLEANFILES = $(top_srcdir)/zmapAppServices/Makefile.in

