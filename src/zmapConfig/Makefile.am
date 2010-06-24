
include $(top_srcdir)/Makefile.common

SUBDIRS = .

noinst_LTLIBRARIES = libZMapConfig.la

libZMapConfig_la_SOURCES = zmapConfigDir.c \
zmapConfigDir_P.h \
zmapConfigFile.c \
zmapConfigIni.c \
zmapConfigIni_P.h \
zmapConfigKey.c \
zmapConfigLoader.c \
$(NULL)

libZMapConfig_la_CFLAGS = $(INCLUDE_COMMON)
