
include $(top_srcdir)/Makefile.common

SUBDIRS = .

noinst_LTLIBRARIES = libSamtoolsExamples.la

libSamtoolsExamples_la_SOURCES = bam2bed.c calDepth.c chk_indel.c
libSamtoolsExamples_la_CFLAGS  = $(INCLUDE_COMMON) -I$(top_srcdir)/samtools_parallel/examples
libSamtoolsExamples_la_LIBADD  = 


# Extra files to remove for the maintainer-clean target.
#
MAINTAINERCLEANFILES = $(top_srcdir)/Ensembl/src/samtools_parallel/examples/Makefile.in
