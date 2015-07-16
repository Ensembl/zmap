
include $(top_srcdir)/Makefile.zmap

SUBDIRS = .

noinst_LTLIBRARIES = libZMapGFF.la

libZMapGFF_la_SOURCES = zmapGFF_P.hpp \
zmapGFF2_P.hpp \
zmapGFF3_P.hpp \
zmapGFF2Dumper.cpp \
zmapGFF2parser.cpp \
zmapGFF3parser.cpp \
zmapGFFGeneral.cpp \
zmapGFFUtils.cpp \
zmapGFFAttribute.hpp \
zmapGFFAttribute.cpp \
zmapGFFDirective.hpp \
zmapGFFDirective.cpp \
zmapGFFFeatureData.hpp \
zmapGFFFeatureData.cpp \
zmapGFFHeader.hpp \
zmapGFFHeader.cpp \
zmapGFFStringUtils.cpp \
zmapGFFUtils.cpp \
zmapMLF.cpp \
zmapMLF_P.hpp \
zmapSOData_P.hpp \
zmapSOParser_P.hpp \
zmapSOParser.cpp \
$(NULL)

libZMapGFF_la_CFLAGS = $(INCLUDE_COMMON)


# Extra files to remove for the maintainer-clean target.
#
MAINTAINERCLEANFILES = $(top_srcdir)/zmapGFF/Makefile.in



