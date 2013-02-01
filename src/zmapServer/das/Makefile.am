
include $(top_srcdir)/Makefile.common

SUBDIRS = .

noinst_LTLIBRARIES = libDasServer.la

libDasServer_la_SOURCES = dasServer.c dasServer_P.h
libDasServer_la_LIBADD  = 
libDasServer_la_CFLAGS  = $(INCLUDE_COMMON) -I$(top_srcdir)/zmapServer
libDasServer_la_LDFLAGS =

# Extra files to remove for the maintainer-clean target.
#
MAINTAINERCLEANFILES = $(top_srcdir)/zmapServer/DAS/Makefile.in



