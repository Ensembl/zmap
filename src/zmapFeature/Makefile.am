
include $(top_srcdir)/Makefile.zmap

SUBDIRS = . tests

noinst_LTLIBRARIES = libZMapFeature.la

libZMapFeature_la_SOURCES =    \
zmapFeature.c                  \
zmapFeature_P.h                \
zmapFeature3FrameTranslation.c \
zmapFeatureAlignment.c         \
zmapFeatureBasic.c              \
zmapFeatureContext.c           \
zmapFeatureContextUtils.c      \
zmapFeatureDNA.c               \
zmapFeatureFormatInput.c       \
zmapFeatureData.c   \
zmapFeatureOutput.c \
zmapFeatureParams.c \
zmapFeatureTranscript.c        \
zmapFeatureUtils.c  \
zmapStyle.c         \
zmapStyle_P.h       \
zmapStyleObj.c  \
zmapStyleUtils.c    \
$(NULL)

libZMapFeature_la_CFLAGS = $(INCLUDE_COMMON) -I$(top_srcdir)/zmapUtils



# Extra files to remove for the maintainer-clean target.
#
MAINTAINERCLEANFILES = $(top_srcdir)/zmapFeature/Makefile.in


