
include $(top_srcdir)/Makefile.common

SUBDIRS = .

noinst_LTLIBRARIES = libZMapXML.la

libZMapXML_la_SOURCES =  zmapXMLAttribute.c \
zmapXMLDocument.c \
zmapXMLElement.c \
zmapXMLParse.c \
zmapXMLUtils.c \
zmapXMLWriter.c \
zmapXML_P.h \
$(NULL)

libZMapXML_la_CFLAGS = $(INCLUDE_COMMON)

# Extra files to remove for the maintainer-clean target.
#
MAINTAINERCLEANFILES = $(top_srcdir)/zmapXML/Makefile.in




