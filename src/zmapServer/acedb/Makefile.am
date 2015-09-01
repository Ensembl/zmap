
include $(top_srcdir)/Makefile.zmap

SUBDIRS = .

AM_CPPFLAGS += -I$(top_srcdir)/zmapServer

noinst_LTLIBRARIES = libAcedbServer.la

libAcedbServer_la_SOURCES = acedbServer.cpp acedbServer_P.hpp
libAcedbServer_la_LIBADD  = 
libAcedbServer_la_CFLAGS  = $(INCLUDE_COMMON) -I$(top_srcdir)/zmapServer


# Extra files to remove for the maintainer-clean target.
#
MAINTAINERCLEANFILES = $(top_srcdir)/zmapServer/acedb/Makefile.in



