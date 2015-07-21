
include $(top_srcdir)/Makefile.common

check_LTLIBRARIES =      \
libcheckZMapFeature.la   \
libcheckZMapFeature_P.la \
libcheckZMapStyle.la     \
libcheckZMapStyle_P.la   \
$(NULL)

libcheckZMapFeature_la_SOURCES = check_zmapFeature.cpp check_zmapFeature.hpp
libcheckZMapFeature_la_LIBADD  = 
libcheckZMapFeature_la_CFLAGS  = $(INCLUDE_COMMON) @CHECK_CFLAGS@

libcheckZMapFeature_P_la_SOURCES = check_zmapFeature_P.cpp check_zmapFeature_P.hpp
libcheckZMapFeature_P_la_LIBADD  = 
libcheckZMapFeature_P_la_CFLAGS  = $(INCLUDE_COMMON) @CHECK_CFLAGS@ -I$(top_srcdir)/zmapFeature

libcheckZMapStyle_la_SOURCES = check_zmapStyle.cpp check_zmapStyle.hpp
libcheckZMapStyle_la_LIBADD  = 
libcheckZMapStyle_la_CFLAGS  = $(INCLUDE_COMMON) @CHECK_CFLAGS@

libcheckZMapStyle_P_la_SOURCES = check_zmapStyle_P.cpp check_zmapStyle_P.hpp
libcheckZMapStyle_P_la_LIBADD  = 
libcheckZMapStyle_P_la_CFLAGS  = $(INCLUDE_COMMON) @CHECK_CFLAGS@ -I$(top_srcdir)/zmapFeature
