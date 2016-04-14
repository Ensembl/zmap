
include $(top_srcdir)/Makefile.common

SUBDIRS = .

AM_CPPFLAGS += -I$(top_srcdir)/zmapServer -I@MYSQL_LOCATION@ $(libTrackhubServer_la_CFLAGS)

noinst_LTLIBRARIES = libTrackhubServer.la

libTrackhubServer_la_SOURCES = trackhubServer.cpp trackhubServer_P.hpp
libTrackhubServer_la_LIBADD  = 
libTrackhubServer_la_CFLAGS  = $(INCLUDE_COMMON) -I$(top_srcdir)/zmapServer $(MYSQL_CFLAGS)
libTrackhubServer_la_LDFLAGS = -lensc $(MYSQL_LDFLAGS)


# Extra files to remove for the maintainer-clean target.
#
MAINTAINERCLEANFILES = $(top_srcdir)/zmapServer/trackhub/Makefile.in
