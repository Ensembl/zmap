
include $(top_srcdir)/Makefile.common

SUBDIRS = .

noinst_LTLIBRARIES = libZMapThreadsLib.la

libZMapThreadsLib_la_SOURCES = \
zmapThreads.cpp \
zmapThreadsUtils.cpp \
zmapThreads_P.hpp \
$(NULL)

libZMapThreadsLib_la_LIBADD  = 
libZMapThreadsLib_la_CFLAGS  = $(INCLUDE_COMMON)
libZMapThreadsLib_la_LDFLAGS = 


# Extra files to remove for the maintainer-clean target.
#
MAINTAINERCLEANFILES = $(top_srcdir)/zmapThreadLib/Makefile.in


