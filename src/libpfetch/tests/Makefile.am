
include $(top_srcdir)/Makefile.common

SUBDIRS = .

check_LTLIBRARIES = libcheckPFetch.la

libcheckPFetch_la_SOURCES = check_libpfetch.c check_libpfetch.h
libcheckPFetch_la_LIBADD  = 
libcheckPFetch_la_CFLAGS  = $(INCLUDE_COMMON) -DCHECK_PFETCH_LOCATION=\"`pwd`/pfetch.sh\" @CHECK_CFLAGS@
libcheckPFetch_la_DEPENDENCIES = pfetch.sh

pfetch.sh:
	touch pfetch.sh
	chmod 755 pfetch.sh
	echo -e '#!/bin/sh \ncat - <<PFETCH\n>Te:stSequence_1.20\natgctgatcgatgcatgcat\nPFETCH\n' >> pfetch.sh
