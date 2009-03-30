
include $(top_srcdir)/Makefile.common

SUBDIRS = .

noinst_LTLIBRARIES = libZMapConfig.la

libZMapConfig_la_SOURCES = zmapConfigUtils.c \
zmapConfig.c \
zmapConfig_P.h \
zmapConfigIni.c \
zmapConfigRead.c \
zmapConfigGet.c \
zmapConfigWrite.c \
$(NULL)

libZMapConfig_la_CFLAGS = $(INCLUDE_COMMON)
