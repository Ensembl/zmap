
include $(top_srcdir)/Makefile.common

SUBDIRS = .

noinst_LTLIBRARIES = libFileServer.la

libFileServer_la_SOURCES = fileServer.c fileServer_P.h
libFileServer_la_LIBADD  = 
libFileServer_la_CFLAGS  = $(INCLUDE_COMMON) -I$(top_srcdir)/zmapServer
