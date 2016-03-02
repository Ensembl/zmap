
include $(top_srcdir)/Makefile.common

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
MAINTAINERCLEANFILES = $(top_srcdir)/zmapConfig/Makefile.in $(top_srcdir)/zmapConfig/default_styles.cpp

# PLEASE READ:
# We would like to create the default_styles.cpp file here but the below mechanism which tries to
# make use of the automake provided target "all-local" cannot be made to work reliably. This is
# because there is NO guarantee about when all-local gets run, it can end up being run _after_
# the "all" target so then the "all" target fails because there is no default_styles.cpp file.
#
# As a not very satisfactory hack I've moved the code to create default_styles.cpp to the
# zmapApp directory because targets there are always run before this directory.
#
#default_styles.cpp: $(top_srcdir)/zmapConfig/default_styles.txt
#	@echo 'about to make default_styles.cpp'
#	$(top_srcdir)/zmapConfig/zmap_DefaultStyles.pl $(top_srcdir)/zmapConfig/default_styles.txt > $(top_srcdir)/zmapConfig/default_styles.cpp
#	@echo 'made default_styles.cpp'
#
#default-styles: $(top_srcdir)/zmapConfig/default_styles.cpp
#
#all-local: default-styles


