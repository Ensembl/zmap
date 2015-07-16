
include $(top_srcdir)/Makefile.zmap

SUBDIRS = .

noinst_LTLIBRARIES = libZMapConfig.la

libZMapConfig_la_SOURCES = zmapConfigDir.cpp \
zmapConfigDir_P.hpp \
zmapConfigFile.cpp \
zmapConfigIni.cpp \
zmapConfigIni_P.hpp \
zmapConfigKey.cpp \
zmapConfigLoader.cpp \
default_styles.txt\
default_styles.cpp\
zmap_DefaultStyles.pl\
$(NULL)

libZMapConfig_la_CFLAGS = $(INCLUDE_COMMON)


# Extra files to remove for the maintainer-clean target.
#
MAINTAINERCLEANFILES = $(top_srcdir)/zmapConfig/Makefile.in



# Create default styles string dynamically each time the text file is changed.
#
default_styles.cpp: $(top_srcdir)/zmapConfig/default_styles.txt
	$(top_srcdir)/zmapConfig/zmap_DefaultStyles.pl $(top_srcdir)/zmapConfig/default_styles.txt > $(top_srcdir)/zmapConfig/default_styles.cpp

default-styles: $(top_srcdir)/zmapConfig/default_styles.cpp

all-local: default-styles

