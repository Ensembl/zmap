
include $(top_srcdir)/Makefile.zmap

SUBDIRS = .

AM_CPPFLAGS += -I$(top_srcdir)/zmapServer

noinst_LTLIBRARIES = libFileServer.la

libFileServer_la_SOURCES = fileServer.cpp fileServer_P.hpp
libFileServer_la_LIBADD  = 
libFileServer_la_LDFLAGS =
libFileServer_la_CFLAGS  = $(INCLUDE_COMMON) -I$(top_srcdir)/zmapServer

# Extra files to remove for the maintainer-clean target.
#
MAINTAINERCLEANFILES = $(top_srcdir)/zmapServer/file/Makefile.in



