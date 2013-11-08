
include $(top_srcdir)/Makefile.common

SUBDIRS = .

noinst_LTLIBRARIES = libZMapGFF.la

libZMapGFF_la_SOURCES = zmapGFF_P.h \
zmapGFF2_P.h \
zmapGFF3_P.h \
zmapGFF2Dumper.c \
zmapGFF2parser.c \
zmapGFF3parser.c \
zmapGFFGeneral.c \
zmapGFFUtils.c \
zmapGFFAttribute.h \
zmapGFFAttribute.c \
zmapGFFDirective.h \
zmapGFFDirective.c \
zmapGFFFeatureData.h \
zmapGFFFeatureData.c \
zmapGFFHeader.h \
zmapGFFHeader.c \
zmapGFFStringUtils.c \
zmapGFFUtils.c \
zmapMLF.c \
zmapMLF_P.h \
zmapSOData_P.h \
zmapSOParser.h \
zmapSOParser.c \
$(NULL)

libZMapGFF_la_CFLAGS = $(INCLUDE_COMMON)


# Extra files to remove for the maintainer-clean target.
#
MAINTAINERCLEANFILES = $(top_srcdir)/zmapGFF/Makefile.in



