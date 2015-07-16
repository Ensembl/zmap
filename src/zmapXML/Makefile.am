
include $(top_srcdir)/Makefile.zmap

SUBDIRS = .

noinst_LTLIBRARIES = libZMapXML.la

libZMapXML_la_SOURCES =  zmapXMLAttribute.cpp \
zmapXMLDocument.cpp \
zmapXMLElement.cpp \
zmapXMLParse.cpp \
zmapXMLUtils.cpp \
zmapXMLWriter.cpp \
zmapXML_P.hpp \
$(NULL)

libZMapXML_la_CFLAGS = $(INCLUDE_COMMON)

# Extra files to remove for the maintainer-clean target.
#
MAINTAINERCLEANFILES = $(top_srcdir)/zmapXML/Makefile.in




