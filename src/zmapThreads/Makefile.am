
include $(top_srcdir)/Makefile.common

SUBDIRS = .

noinst_LTLIBRARIES = libZMapThreads.la

libZMapThreads_la_SOURCES = zmapSlave.cpp \
zmapSlave_P.hpp \
zmapThreads.cpp \
zmapThreads_P.hpp \
zmapThreadsUtils.cpp \
$(NULL)
libZMapThreads_la_LIBADD  = 
libZMapThreads_la_CFLAGS  = $(INCLUDE_COMMON)
libZMapThreads_la_LDFLAGS = 


# Extra files to remove for the maintainer-clean target.
#
MAINTAINERCLEANFILES = $(top_srcdir)/zmapThread/Makefile.in


