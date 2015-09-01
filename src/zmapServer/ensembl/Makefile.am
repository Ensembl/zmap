
include $(top_srcdir)/Makefile.zmap

SUBDIRS = .

AM_CPPFLAGS += -I$(top_srcdir)/zmapServer -I@MYSQL_LOCATION@ $(libEnsemblServer_la_CFLAGS)

noinst_LTLIBRARIES = libEnsemblServer.la

libEnsemblServer_la_SOURCES = ensemblServer.cpp ensemblServer_P.hpp
libEnsemblServer_la_LIBADD  = 
libEnsemblServer_la_CFLAGS  = $(INCLUDE_COMMON) -I$(top_srcdir)/zmapServer $(MYSQL_CFLAGS)
libEnsemblServer_la_LDFLAGS = -lensc $(MYSQL_LDFLAGS)


# Extra files to remove for the maintainer-clean target.
#
MAINTAINERCLEANFILES = $(top_srcdir)/zmapServer/ensembl/Makefile.in
