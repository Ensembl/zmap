
include $(top_srcdir)/Makefile.common

check_LTLIBRARIES =        \
libcheckZMapUtilsConfig.la \
$(NULL)


libcheckZMapUtilsConfig_la_location = $(top_srcdir)/share/ZMap
libcheckZMapUtilsConfig_la_filename = ZMap

libcheckZMapUtilsConfig_la_SOURCES = check_zmapConfigLoader.c check_zmapConfigLoader.h
libcheckZMapUtilsConfig_la_LIBADD  = 
libcheckZMapUtilsConfig_la_CFLAGS  = $(INCLUDE_COMMON) @CHECK_CFLAGS@ \
-DCONFIG_DIR=\"$(libcheckZMapUtilsConfig_la_location)\"               \
-DCONFIG_FILE=\"$(libcheckZMapUtilsConfig_la_filename)\"              \
$(NULL)
