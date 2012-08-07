
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
default_styles.c\
default_styles.txt\
gen_default\
$(NULL)

libZMapConfig_la_CFLAGS = $(INCLUDE_COMMON)

# refer to main src/Makefile.am default-style