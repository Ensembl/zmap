
include $(top_srcdir)/Makefile.zmap

SUBDIRS = .

# hacked to make mac build work...this is not the way to do it...
AM_CPPFLAGS += -I$(top_srcdir)/zmapServer -I/usr/include/mysql -I/opt/local/include/mysql55

noinst_LTLIBRARIES = libEnsemblServer.la

libEnsemblServer_la_SOURCES = ensemblServer.cpp ensemblServer_P.hpp
libEnsemblServer_la_LIBADD  = 
libEnsemblServer_la_CFLAGS  = $(INCLUDE_COMMON) -I$(top_srcdir)/zmapServer $(MYSQL_CFLAGS)
libEnsemblServer_la_LDFLAGS = -lensc $(MYSQL_LDFLAGS)


# Extra files to remove for the maintainer-clean target.
#
MAINTAINERCLEANFILES = $(top_srcdir)/zmapServer/ensembl/Makefile.in
