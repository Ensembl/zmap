
include $(top_srcdir)/Makefile.common

SUBDIRS = .

noinst_LTLIBRARIES = libZMapWindow.la libZMapWindowGUI.la libZMapWindowItems.la libZMapWindowCanvas.la

libZMapWindow_la_SOURCES =
libZMapWindow_la_LIBADD  = libZMapWindowGUI.la libZMapWindowItems.la libZMapWindowCanvas.la
libZMapWindow_la_LDFLAGS = -all-static

libZMapWindowGUI_la_SOURCES = \
zmapWindow.c \
zmapWindowColBump.c \
zmapWindowColConfig.c \
zmapWindowColOrder.c \
zmapWindowDNA.c \
zmapWindowDNAChoose.c \
zmapWindowDNAList.c \
zmapWindowDNAList.h \
zmapWindowDNAList_I.h \
zmapWindowDraw.c \
zmapWindowDrawFeatures.c \
zmapWindowDump.c \
zmapWindowExport.c \
zmapWindowFeature.c \
zmapWindowFeatureFuncs.c \
zmapWindowFeatureList.c \
zmapWindowFeatureList.h \
zmapWindowFeatureList_I.h \
zmapWindowFeatureShow.c \
zmapWindowFocus.c \
zmapWindowFrame.c \
zmapWindowItem.c \
zmapWindowItemDebug.c \
zmapWindowItemHash.c \
zmapWindowList.c \
zmapWindowMark.c \
zmapWindowMark_P.h \
zmapWindowMenus.c \
zmapWindowNavigator.c \
zmapWindowNavigatorMenus.c \
zmapWindowNavigatorWidget.c \
zmapWindowNavigator_P.h \
zmapWindowOverlays.c \
zmapWindowOverlays.h \
zmapWindowPreferences.c \
zmapWindowPrint.c \
zmapWindowRemoteControl.c \
zmapWindowRemoteControlUtils.c \
zmapWindowRemoteReceive.c \
zmapWindowScale.c \
zmapWindowSearch.c \
zmapWindowSeqDisp.c \
zmapWindowState.c \
zmapWindowState.h \
zmapWindowStats.c \
zmapWindowStyle.c \
zmapWindowUtils.c \
zmapWindowZoomControl.c \
zmapWindowZoomControl_P.h \
zmapWindow_P.h \
$(NULL)

libZMapWindowGUI_la_CFLAGS = $(INCLUDE_COMMON) -I$(top_srcdir)/zmapUtils -I$(top_srcdir)/zmapWindow/items -I$(top_srcdir)/zmapWindow/canvas

libZMapWindowCanvas_la_SOURCES = \
canvas/zmapWindowCanvasAlignment.c \
canvas/zmapWindowCanvasAlignment.h \
canvas/zmapWindowCanvasAlignment_I.h \
canvas/zmapWindowCanvasAssembly.c \
canvas/zmapWindowCanvasAssembly.h \
canvas/zmapWindowCanvasAssembly_I.h \
canvas/zmapWindowCanvasBasic.c \
canvas/zmapWindowCanvasBasic.h \
canvas/zmapWindowCanvasFeatureset.c \
canvas/zmapWindowCanvasFeaturesetBump.c \
canvas/zmapWindowCanvasFeaturesetSummarise.c \
canvas/zmapWindowCanvasFeatureset_I.h \
canvas/zmapWindowCanvasFeatureset.h \
canvas/zmapWindowCanvasGlyph.c \
canvas/zmapWindowCanvasGlyph.h \
canvas/zmapWindowCanvasGlyph_I.h \
canvas/zmapWindowCanvasGraphItem.c \
canvas/zmapWindowCanvasGraphItem.h \
canvas/zmapWindowCanvasGraphItem_I.h \
canvas/zmapWindowCanvasTranscript.c \
canvas/zmapWindowCanvasTranscript.h \
canvas/zmapWindowCanvasTranscript_I.h \
canvas/zmapWindowCanvasSequence.c \
canvas/zmapWindowCanvasSequence.h \
canvas/zmapWindowCanvasSequence_I.h \
canvas/zmapWindowCanvasLocus.c \
canvas/zmapWindowCanvasLocus.h \
canvas/zmapWindowCanvasLocus_I.h \
canvas/zmapWindowCanvasGraphics.c \
canvas/zmapWindowCanvasGraphics.h \
canvas/zmapWindowCanvasGraphics_I.h \
$(NULL)


libZMapWindowItems_la_SOURCES = \
items/zmapWindowAllBase.c \
items/zmapWindowAllBase.h \
items/zmapWindowCanvas.c \
items/zmapWindowCanvas.h \
items/zmapWindowCanvasItem.c \
items/zmapWindowCanvasItem.h \
items/zmapWindowCanvasItem_I.h \
items/zmapWindowCanvas_I.h \
items/zmapWindowContainerAlignment.c \
items/zmapWindowContainerAlignment.h \
items/zmapWindowContainerAlignment_I.h \
items/zmapWindowContainerBlock.c \
items/zmapWindowContainerBlock.h \
items/zmapWindowContainerBlock_I.h \
items/zmapWindowContainerChildren.c \
items/zmapWindowContainerChildren.h \
items/zmapWindowContainerChildren_I.h \
items/zmapWindowContainerContext.c \
items/zmapWindowContainerContext.h \
items/zmapWindowContainerContext_I.h \
items/zmapWindowContainerFeatureSet.c \
items/zmapWindowContainerFeatureSet.h \
items/zmapWindowContainerFeatureSet_I.h \
items/zmapWindowContainerGroup.c \
items/zmapWindowContainerGroup.h \
items/zmapWindowContainerGroup_I.h \
items/zmapWindowContainerStrand.c \
items/zmapWindowContainerStrand.h \
items/zmapWindowContainerStrand_I.h \
items/zmapWindowContainerUtils.c \
items/zmapWindowContainerUtils.h \
items/zmapWindowContainerUtils_P.h \
items/zmapWindowContainers.h \
items/zmapWindowContainers_I.h \
$(NULL)

# We need the top_builddir to find the generated marshal code
libZMapWindowItems_la_CFLAGS = $(INCLUDE_COMMON) -I$(top_srcdir)/zmapUtils -I$(top_srcdir)/zmapWindow/items -I$(top_srcdir)/zmapWindow/canvas

libZMapWindowItems_la_DEPENDENCIES = $(nodist_libZMapWindowItems_la_SOURCES)



libZMapWindowCanvas_la_CFLAGS = $(INCLUDE_COMMON) -I$(top_srcdir)/zmapUtils  -I$(top_srcdir)/zmapWindow/canvas -I$(top_srcdir)/zmapWindow/items

libZMapWindowCanvas_la_DEPENDENCIES = $(nodist_libZMapWindowCanvas_la_SOURCES)


