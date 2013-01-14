
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
default_styles.txt\
default_styles.c\
gen_default\
$(NULL)

libZMapConfig_la_CFLAGS = $(INCLUDE_COMMON)


# Extra files to remove for the maintainer-clean target.
#
MAINTAINERCLEANFILES = $(top_srcdir)/zmapConfig/Makefile.in



# Create default styles string dynamically each time the text file is changed.
#
$(top_srcdir)/zmapConfig/default_styles.c: $(top_srcdir)/zmapConfig/default_styles.txt
	$(top_srcdir)/zmapConfig/gen_default $(top_srcdir)/zmapConfig/default_styles.txt > $(top_srcdir)/zmapConfig/default_styles.c

default-styles: $(top_srcdir)/zmapConfig/default_styles.c

all-local: default-styles

