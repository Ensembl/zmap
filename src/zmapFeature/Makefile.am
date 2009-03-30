
include $(top_srcdir)/Makefile.common

SUBDIRS = .

noinst_LTLIBRARIES = libZMapFeature.la

libZMapFeature_la_SOURCES = zmapFeatureTypes.c \
zmapStyle_P.h \
zmapFeatureOutput.c \
zmapFeatureFormatInput.c \
zmapFeature.c \
zmapFeatureXML.c \
zmapFeatureUtils.c \
zmapStyle.c \
zmapFeature_P.h \
zmapStyle_I.h \
zmapFeatureContextUtils.c \
zmapFeatureContext.c \
zmapStyleUtils.c \
$(NULL)

libZMapFeature_la_CFLAGS = $(INCLUDE_COMMON) -I$(top_srcdir)/zmapUtils
