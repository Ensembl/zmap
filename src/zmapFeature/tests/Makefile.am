
include $(top_srcdir)/Makefile.common

check_LTLIBRARIES =      \
libcheckZMapFeature.la   \
libcheckZMapFeature_P.la \
libcheckZMapStyle.la     \
libcheckZMapStyle_P.la   \
$(NULL)

libcheckZMapFeature_la_SOURCES = check_zmapFeature.c check_zmapFeature.h
libcheckZMapFeature_la_LIBADD  = 
libcheckZMapFeature_la_CFLAGS  = $(INCLUDE_COMMON) @CHECK_CFLAGS@

libcheckZMapFeature_P_la_SOURCES = check_zmapFeature_P.c check_zmapFeature_P.h
libcheckZMapFeature_P_la_LIBADD  = 
libcheckZMapFeature_P_la_CFLAGS  = $(INCLUDE_COMMON) @CHECK_CFLAGS@ -I$(top_srcdir)/zmapFeature

libcheckZMapStyle_la_SOURCES = check_zmapStyle.c check_zmapStyle.h
libcheckZMapStyle_la_LIBADD  = 
libcheckZMapStyle_la_CFLAGS  = $(INCLUDE_COMMON) @CHECK_CFLAGS@

libcheckZMapStyle_P_la_SOURCES = check_zmapStyle_P.c check_zmapStyle_P.h
libcheckZMapStyle_P_la_LIBADD  = 
libcheckZMapStyle_P_la_CFLAGS  = $(INCLUDE_COMMON) @CHECK_CFLAGS@ -I$(top_srcdir)/zmapFeature
