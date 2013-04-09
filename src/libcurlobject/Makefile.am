
include $(top_srcdir)/Makefile.common

SUBDIRS = .

noinst_LTLIBRARIES = libcurlobject.la

libcurlobject_la_SOURCES = libcurlobject.c \
libcurlobject.h \
libcurlobject_I.h \
$(NULL)  

libcurlobject_la_CPPFLAGS = $(INCLUDE_COMMON) -I$(AM_CPPFLAGS)



# Extra files to remove for the maintainer-clean target.
#
MAINTAINERCLEANFILES = $(top_srcdir)/libcurlobject/Makefile.in
