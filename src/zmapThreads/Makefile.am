
include $(top_srcdir)/Makefile.common

SUBDIRS = .

noinst_LTLIBRARIES = libZMapThreads.la

libZMapThreads_la_SOURCES = zmapSlave.c \
zmapSlave_P.h \
zmapThreads.c \
zmapThreads_P.h \
zmapThreadsUtils.c \
$(NULL)
libZMapThreads_la_LIBADD  = 
libZMapThreads_la_CFLAGS  = $(INCLUDE_COMMON)
libZMapThreads_la_LDFLAGS = 


# Extra files to remove for the maintainer-clean target.
#
MAINTAINERCLEANFILES = $(top_srcdir)/zmapThread/Makefile.in


