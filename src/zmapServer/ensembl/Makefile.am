
include $(top_srcdir)/Makefile.common

SUBDIRS = .

noinst_LTLIBRARIES = libEnsemblServer.la

libEnsemblServer_la_SOURCES = ensemblServer.c ensemblServer_P.h
libEnsemblServer_la_LIBADD  = 
libEnsemblServer_la_CFLAGS  = $(INCLUDE_COMMON) -I$(top_srcdir)/zmapServer


# Extra files to remove for the maintainer-clean target.
#
MAINTAINERCLEANFILES = $(top_srcdir)/zmapServer/ensembl/Makefile.in



