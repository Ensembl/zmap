
include $(top_srcdir)/Makefile.common

SUBDIRS = . tests

noinst_LTLIBRARIES = libZMapFeature.la

libZMapFeature_la_SOURCES =  	 \
zmapFeature.cpp                  \
zmapFeature_P.hpp                \
zmapFeature3FrameTranslation.cpp \
zmapFeatureAlignment.cpp         \
zmapFeatureAny.cpp		 \
zmapFeatureBasic.cpp             \
zmapFeatureContext.cpp           \
zmapFeatureContextAlign.cpp	\
zmapFeatureContextBlock.cpp	\
zmapFeatureContextSet.cpp	\
zmapFeatureContextUtils.cpp      \
zmapFeatureDNA.cpp               \
zmapFeatureFormatInput.cpp       \
zmapFeatureData.cpp   \
zmapFeatureOutput.cpp \
zmapFeatureParams.cpp \
zmapFeatureTranscript.cpp        \
zmapFeatureUtils.cpp  \
zmapStyle.cpp         \
zmapStyleTree.cpp         \
zmapStyle_P.hpp       \
zmapStyleObj.cpp  \
zmapStyleUtils.cpp    \
zmapFeatureLoadDisplay.cpp \
$(NULL)

libZMapFeature_la_CFLAGS = $(INCLUDE_COMMON) -I$(top_srcdir)/zmapUtils



# Extra files to remove for the maintainer-clean target.
#
MAINTAINERCLEANFILES = $(top_srcdir)/zmapFeature/Makefile.in


