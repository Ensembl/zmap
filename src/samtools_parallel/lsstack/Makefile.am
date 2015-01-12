
include $(top_srcdir)/Makefile.common

SUBDIRS = .

noinst_LTLIBRARIES = libSamtoolsLsstack.la

libSamtoolsLsstack_la_SOURCES = lsstack.c
libSamtoolsLsstack_la_CFLAGS  = $(INCLUDE_COMMON) -I$(top_srcdir)/samtools_parallel/lsstack
libSamtoolsLsstack_la_LIBADD  = 


# Extra files to remove for the maintainer-clean target.
#
MAINTAINERCLEANFILES = $(top_srcdir)/Ensembl/src/samtools_parallel/lsstack/Makefile.in
