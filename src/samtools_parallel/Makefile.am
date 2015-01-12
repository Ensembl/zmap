
include $(top_srcdir)/Makefile.common

SUBDIRS = examples lsstack misc pgbzip pstack-1.2 win32

noinst_LTLIBRARIES = libSamtoolsParallel.la

libSamtoolsParallel_la_SOURCES = bam.c bam.h bam2bcf.c bam2bcf.h bam2bcf_indel.c bam2depth.c bam_aux.c bam_cat.c bam_color.c bam_endian.h bam_import.c bam_index.c bam_lpileup.c bam_mate.c bam_md.c bam_pileup.c bam_plcmd.c bam_qa.c bam_reheader.c bam_rmdup.c bam_rmdupse.c bam_sort.c bam_stat.c bam_tview.c bamshuf.c bamtk.c bedcov.c bedidx.c bgzf.c bgzf.h bgzip.c cut_target.c errmod.c errmod.h faidx.c faidx.h kaln.c kaln.h khash.h klist.h knetfile.c knetfile.h kprobaln.c kprobaln.h kseq.h ksort.h kstring.c kstring.h padding.c phase.c radix.h razf.c razf.h razip.c sam.c sam.h sam_header.c sam_header.h sam_view.c sample.c sample.h
libSamtoolsParallel_la_CFLAGS  = $(INCLUDE_COMMON) -I$(top_srcdir)/samtools_parallel
libSamtoolsParallel_la_LIBADD  = \
examples/libSamtoolsExamples.la \
lsstack/libSamtoolsLsstack.la   \
misc/libSamtoolsMisc.la         \
pgbzip/libSamtoolsPgbzip.la       \
pstack-1.2/libSamtoolsPsstack.la \
win32/libSamtoolsWin32.la         \
$(NULL)


# Extra files to remove for the maintainer-clean target.
#
MAINTAINERCLEANFILES = $(top_srcdir)/Ensembl/src/samtools_parallel/Makefile.in
