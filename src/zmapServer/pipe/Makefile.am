
include $(top_srcdir)/Makefile.common

SUBDIRS = .

AM_CPPFLAGS += -I$(top_srcdir)/zmapServer

noinst_LTLIBRARIES = libPipeServer.la

libPipeServer_la_SOURCES = pipeServer.cpp pipeServer_P.hpp
libPipeServer_la_LIBADD  = 
libPipeServer_la_CFLAGS  = $(INCLUDE_COMMON) -I$(top_srcdir)/zmapServer

# Extra files to remove for the maintainer-clean target.
#
MAINTAINERCLEANFILES = $(top_srcdir)/zmapServer/pipe/Makefile.in



