
include $(top_srcdir)/Makefile.common

SUBDIRS = . tests

noinst_LTLIBRARIES = libZMapFeature.la

libZMapFeature_la_SOURCES =    \
zmapFeature.c                  \
zmapFeature_P.h                \
zmapFeature3FrameTranslation.c \
zmapFeatureAlignment.c         \
zmapFeatureTranscript.c        \
zmapFeatureContext.c           \
zmapFeatureContextUtils.c      \
zmapFeatureDNA.c               \
zmapFeatureFormatInput.c       \
zmapFeatureData.c   \
zmapFeatureOutput.c \
zmapFeatureTypes.c  \
zmapFeatureUtils.c  \
zmapStyle.c         \
zmapStyle_I.h       \
zmapStyle_P.h       \
zmapStyleUtils.c    \
$(NULL)

libZMapFeature_la_CFLAGS = $(INCLUDE_COMMON) -I$(top_srcdir)/zmapUtils



# Extra files to remove for the maintainer-clean target.
#
MAINTAINERCLEANFILES = $(top_srcdir)/zmapFeature/Makefile.in


