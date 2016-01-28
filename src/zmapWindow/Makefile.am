
include $(top_srcdir)/Makefile.common

SUBDIRS = .

#noinst_LTLIBRARIES = libZMapWindow.la libZMapWindowGUI.la libZMapWindowItems.la libZMapWindowCanvas.la
noinst_LTLIBRARIES = libZMapWindow.la libZMapWindowGUI.la libZMapWindowCanvas.la

libZMapWindow_la_SOURCES =
#libZMapWindow_la_LIBADD  = libZMapWindowGUI.la libZMapWindowItems.la libZMapWindowCanvas.la
libZMapWindow_la_LIBADD  = libZMapWindowGUI.la libZMapWindowCanvas.la
libZMapWindow_la_LDFLAGS = -all-static

libZMapWindowGUI_la_SOURCES = \
zmapWindow.cpp \
zmapWindowCanvas.cpp \
zmapWindowColBump.cpp \
zmapWindowColConfig.cpp \
zmapWindowColOrder.cpp \
zmapWindowCommand.cpp \
zmapWindowDisplayOps.cpp \
zmapWindowDNA.cpp \
zmapWindowDNAChoose.cpp \
zmapWindowDNAList.cpp \
zmapWindowDNAList.hpp \
zmapWindowDNAList_I.hpp \
zmapWindowDraw.cpp \
zmapWindowDrawFeatures.cpp \
zmapWindowDump.cpp \
zmapWindowExport.cpp \
zmapWindowFeature.cpp \
zmapWindowFeatureFilter.cpp \
zmapWindowFeatureFuncs.cpp \
zmapWindowFeatureList.cpp \
zmapWindowFeatureList.hpp \
zmapWindowFeatureList_I.hpp \
zmapWindowFeatureShow.cpp \
zmapWindowFocus.cpp \
zmapWindowItem.cpp \
zmapWindowItemDebug.cpp \
zmapWindowItemHash.cpp \
zmapWindowList.cpp \
zmapWindowMark.cpp \
zmapWindowMark.hpp \
zmapWindowScratch.cpp \
zmapWindowScratch_P.hpp \
zmapWindowMenus.cpp \
zmapWindowNavigator.cpp \
zmapWindowNavigatorMenus.cpp \
zmapWindowNavigatorWidget.cpp \
zmapWindowNavigator_P.hpp \
zmapWindowPreferences.cpp \
zmapWindowPrint.cpp \
zmapWindowRemoteControl.cpp \
zmapWindowRemoteControlUtils.cpp \
zmapWindowScale.cpp \
zmapWindowSearch.cpp \
zmapWindowSeqDisp.cpp \
zmapWindowState.cpp \
zmapWindowState.hpp \
zmapWindowStats.cpp \
zmapWindowStyle.cpp \
zmapWindowStyles.cpp \
zmapWindowUtils.cpp \
zmapWindowZoomControl.cpp \
zmapWindowZoomControl_P.hpp \
zmapWindow_P.hpp \
$(NULL)

#libZMapWindowGUI_la_CFLAGS = $(INCLUDE_COMMON) -I$(top_srcdir)/zmapUtils -I$(top_srcdir)/zmapWindow/items -I$(top_srcdir)/zmapWindow/canvas
libZMapWindowGUI_la_CFLAGS = $(INCLUDE_COMMON) -I$(top_srcdir)/zmapUtils -I$(top_srcdir)/zmapWindow/canvas
libZMapWindowGUI_la_CPPFLAGS = $(AM_CPPFLAGS) $(libZMapWindowGUI_la_CFLAGS)

libZMapWindowCanvas_la_SOURCES = \
canvas/zmapWindowAllBase.hpp \
canvas/zmapWindowCanvasAlignment.cpp \
canvas/zmapWindowCanvasAlignment.hpp \
canvas/zmapWindowCanvasAlignment_I.hpp \
canvas/zmapWindowCanvasAssembly.cpp \
canvas/zmapWindowCanvasAssembly.hpp \
canvas/zmapWindowCanvasAssembly_I.hpp \
canvas/zmapWindowCanvasBasic.cpp \
canvas/zmapWindowCanvasBasic.hpp \
canvas/zmapWindowCanvasDraw.cpp \
canvas/zmapWindowCanvasDraw.hpp \
canvas/zmapWindowCanvasFeature.cpp \
canvas/zmapWindowCanvasFeature.hpp \
canvas/zmapWindowCanvasFeature_I.hpp \
canvas/zmapWindowCanvasFeatureset.cpp \
canvas/zmapWindowCanvasFeatureset.hpp \
canvas/zmapWindowCanvasFeaturesetBump.cpp \
canvas/zmapWindowCanvasFeaturesetSummarise.cpp \
canvas/zmapWindowCanvasFeatureset_I.hpp \
canvas/zmapWindowCanvasGlyph.cpp \
canvas/zmapWindowCanvasGlyph.hpp \
canvas/zmapWindowCanvasGlyph_I.hpp \
canvas/zmapWindowCanvasGraphItem.cpp \
canvas/zmapWindowCanvasGraphItem.hpp \
canvas/zmapWindowCanvasGraphItem_I.hpp \
canvas/zmapWindowCanvasGraphics.cpp \
canvas/zmapWindowCanvasGraphics.hpp \
canvas/zmapWindowCanvasGraphics_I.hpp \
canvas/zmapWindowCanvasItem.hpp \
canvas/zmapWindowCanvasItem_I.hpp \
canvas/zmapWindowCanvasLocus.cpp \
canvas/zmapWindowCanvasLocus.hpp \
canvas/zmapWindowCanvasLocus_I.hpp \
canvas/zmapWindowCanvasSequence.cpp \
canvas/zmapWindowCanvasSequence.hpp \
canvas/zmapWindowCanvasSequence_I.hpp \
canvas/zmapWindowCanvasTranscript.cpp \
canvas/zmapWindowCanvasTranscript.hpp \
canvas/zmapWindowCanvasTranscript_I.hpp \
canvas/zmapWindowContainerAlignment.hpp \
canvas/zmapWindowContainerAlignment_I.hpp \
canvas/zmapWindowContainerBlock.hpp \
canvas/zmapWindowContainerBlock_I.hpp \
canvas/zmapWindowContainerContext.hpp \
canvas/zmapWindowContainerContext_I.hpp \
canvas/zmapWindowContainerFeatureSet.cpp \
canvas/zmapWindowContainerFeatureSetFeatureFuncs.cpp \
canvas/zmapWindowContainerFeatureSet.hpp \
canvas/zmapWindowContainerFeatureSet_I.hpp \
canvas/zmapWindowContainerGroup.cpp \
canvas/zmapWindowContainerGroup.hpp \
canvas/zmapWindowContainerGroup_I.hpp \
canvas/zmapWindowContainerItem.cpp \
canvas/zmapWindowContainerUtils.hpp \
canvas/zmapWindowContainers.hpp \
canvas/zmapWindowOldItems.cpp \
$(NULL)



# We need the top_builddir to find the generated marshal code#
#libZMapWindowItems_la_CFLAGS = $(INCLUDE_COMMON) -I$(top_srcdir)/zmapUtils -I$(top_srcdir)/zmapWindow/items -I$(top_srcdir)/zmapWindow/canvas
#libZMapWindowItems_la_CFLAGS = $(INCLUDE_COMMON) -I$(top_srcdir)/zmapUtils -I$(top_srcdir)/zmapWindow/canvas

#libZMapWindowItems_la_DEPENDENCIES = $(nodist_libZMapWindowItems_la_SOURCES)



#libZMapWindowCanvas_la_CFLAGS = $(INCLUDE_COMMON) -I$(top_srcdir)/zmapUtils  -I$(top_srcdir)/zmapWindow/canvas -I$(top_srcdir)/zmapWindow/items
libZMapWindowCanvas_la_CFLAGS = $(INCLUDE_COMMON) -I$(top_srcdir)/zmapUtils  -I$(top_srcdir)/zmapWindow/canvas
libZMapWindowCanvas_la_CPPFLAGS = $(AM_CPPFLAGS) $(libZMapWindowCanvas_la_CFLAGS)

libZMapWindowCanvas_la_DEPENDENCIES = $(nodist_libZMapWindowCanvas_la_SOURCES)


# Extra files to remove for the maintainer-clean target.
#
MAINTAINERCLEANFILES = $(top_srcdir)/zmapWindow/Makefile.in


