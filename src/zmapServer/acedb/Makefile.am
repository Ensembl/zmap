
include $(top_srcdir)/Makefile.common

SUBDIRS = .

noinst_LTLIBRARIES = libAcedbServer.la

libAcedbServer_la_SOURCES = acedbServer.c acedbServer_P.h
libAcedbServer_la_LIBADD  = 
libAcedbServer_la_CFLAGS  = $(INCLUDE_COMMON) -I$(top_srcdir)/zmapServer


# Extra files to remove for the maintainer-clean target.
#
MAINTAINERCLEANFILES = $(top_srcdir)/zmapServer/acedb/Makefile.in



