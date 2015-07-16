
include $(top_srcdir)/Makefile.zmap

SUBDIRS = .

AM_CPPFLAGS += -I$(top_srcdir)/zmapServer

noinst_LTLIBRARIES = libDasServer.la

libDasServer_la_SOURCES = dasServer.cpp dasServer_P.hpp
libDasServer_la_LIBADD  = 
libDasServer_la_CFLAGS  = $(INCLUDE_COMMON) -I$(top_srcdir)/zmapServer
libDasServer_la_LDFLAGS =

# Extra files to remove for the maintainer-clean target.
#
MAINTAINERCLEANFILES = $(top_srcdir)/zmapServer/DAS/Makefile.in



