
include $(top_srcdir)/Makefile.common

SUBDIRS = .

noinst_LTLIBRARIES = libSamtoolsMisc.la

libSamtoolsMisc_la_SOURCES = ace2sam.c bamcheck.c maq2sam.c md5.c md5.h md5fa.c wgsim.c
libSamtoolsMisc_la_CFLAGS  = $(INCLUDE_COMMON) -I$(top_srcdir)/samtools_parallel/misc
libSamtoolsMisc_la_LIBADD  = 


# Extra files to remove for the maintainer-clean target.
#
MAINTAINERCLEANFILES = $(top_srcdir)/Ensembl/src/samtools_parallel/misc/Makefile.in
