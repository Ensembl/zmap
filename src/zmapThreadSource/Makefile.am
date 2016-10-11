
include $(top_srcdir)/Makefile.common

SUBDIRS = .

noinst_LTLIBRARIES = libZMapThreadSource.la

libZMapThreadSource_la_SOURCES = \
zmapSlave.cpp \
zmapSlave_P.hpp \
zmapThreadSource.cpp \
$(NULL)

libZMapThreadSource_la_LIBADD  = 
libZMapThreadSource_la_CFLAGS  = $(INCLUDE_COMMON)
libZMapThreadSource_la_LDFLAGS = 


# Extra files to remove for the maintainer-clean target.
#
MAINTAINERCLEANFILES = $(top_srcdir)/zmapThreadSource/Makefile.in


