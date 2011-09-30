
include $(top_srcdir)/Makefile.common

SUBDIRS = .

noinst_LTLIBRARIES = libZMapWindow.la libZMapWindowGUI.la libZMapWindowItems.la

libZMapWindow_la_SOURCES =
libZMapWindow_la_LIBADD  = libZMapWindowGUI.la libZMapWindowItems.la
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
zmapWindowItemFactory.c \
zmapWindowItemFactory.h \
zmapWindowItemHash.c \
zmapWindowItemText.c \
zmapWindowItemTextFillColumn.h \
zmapWindowList.c \
zmapWindowLongItems.c \
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
zmapWindowRemoteReceive.c \
zmapWindowRuler.c \
zmapWindowSearch.c \
zmapWindowSeqDisp.c \
zmapWindowState.c \
zmapWindowState.h \
zmapWindowStats.c \
zmapWindowTextPositioner.c \
zmapWindowTextPositioner.h \
zmapWindowUtils.c \
zmapWindowZoomControl.c \
zmapWindowZoomControl_P.h \
zmapWindow_P.h \
$(NULL)

libZMapWindowGUI_la_CFLAGS = $(INCLUDE_COMMON) -I$(top_srcdir)/zmapUtils -I$(top_srcdir)/zmapWindow/items


libZMapWindowItems_la_SOURCES = \
items/zmapWindowAlignmentFeature.c \
items/zmapWindowAlignmentFeature.h \
items/zmapWindowAlignmentFeature_I.h \
items/zmapWindowAllBase.c \
items/zmapWindowAllBase.h \
items/zmapWindowAssemblyFeature.c \
items/zmapWindowAssemblyFeature.h \
items/zmapWindowAssemblyFeature_I.h \
items/zmapWindowBasicFeature.c \
items/zmapWindowBasicFeature.h \
items/zmapWindowBasicFeature_I.h \
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
items/zmapWindowContainerFeatureSetUtils.c \
items/zmapWindowContainerFeatureSet_I.h \
items/zmapWindowContainerGroup.c \
items/zmapWindowContainerGroup.h \
items/zmapWindowContainerGroup_I.h \
items/zmapWindowContainerStrand.c \
items/zmapWindowContainerStrand.h \
items/zmapWindowContainerStrand_I.h \
items/zmapWindowContainerSummarise.c \
items/zmapWindowContainerUtils.c \
items/zmapWindowContainerUtils.h \
items/zmapWindowContainerUtils_P.h \
items/zmapWindowContainers.h \
items/zmapWindowContainers_I.h \
items/zmapWindowFeatures.h \
items/zmapWindowFeatures_I.h \
items/zmapWindowGlyphItem.c \
items/zmapWindowGlyphItem.h \
items/zmapWindowGlyphItem_I.h \
items/zmapWindowGraphItem.c \
items/zmapWindowGraphItem.h \
items/zmapWindowGraphDensityItem_I.h \
items/zmapWindowGraphDensityItem.c \
items/zmapWindowGraphDensityItem.h \
items/zmapWindowGraphItem_I.h \
items/zmapWindowLongItem.c \
items/zmapWindowLongItem.h \
items/zmapWindowLongItem_I.h \
items/zmapWindowLongItems_P.h \
items/zmapWindowSequenceFeature.c \
items/zmapWindowSequenceFeature.h \
items/zmapWindowSequenceFeatureCMarshal.h \
items/zmapWindowSequenceFeature_I.h \
items/zmapWindowTextFeature.c \
items/zmapWindowTextFeature.h \
items/zmapWindowTextFeature_I.h \
items/zmapWindowTextItem.c \
items/zmapWindowTextItem.h \
items/zmapWindowTextItemCMarshal.h \
items/zmapWindowTextItem_I.h \
items/zmapWindowTranscriptFeature.c \
items/zmapWindowTranscriptFeature.h \
items/zmapWindowTranscriptFeature_I.h \
$(NULL)


# We need the top_builddir to find the generated marshal code
libZMapWindowItems_la_CFLAGS = $(INCLUDE_COMMON) -I$(top_srcdir)/zmapUtils \
-I$(top_srcdir)/zmapWindow/items

libZMapWindowItems_la_DEPENDENCIES = $(nodist_libZMapWindowItems_la_SOURCES)

libZMapWindowItems_la_marshal = items/zmapWindowTextItemCMarshal.list \
items/zmapWindowSequenceFeatureCMarshal.list \
$(NULL)

libZMapWindowItems_la_extra_sources = $(srcdir)/items/zmapWindowTextItemCMarshal.h \
$(srcdir)/items/zmapWindowTextItemCMarshal.c \
$(srcdir)/items/zmapWindowSequenceFeatureCMarshal.h \
$(srcdir)/items/zmapWindowSequenceFeatureCMarshal.c \
$(NULL)

nodist_libZMapWindowItems_la_SOURCES = $(libZMapWindowItems_la_extra_sources)

BUILT_SOURCES = $(libZMapWindowItems_la_extra_sources)
CLEANFILES = $(BUILT_SOURCES)
EXTRA_DIST = $(libZMapWindowItems_la_marshal)

# ${*F} _may_ not be available in all makes...
GENMARSHAL_COMMAND = $(GLIB_GENMARSHAL) --prefix=${*F}

.list.h:
	$(GENMARSHAL_COMMAND) --header $< > $@

.list.c:
	$(GENMARSHAL_COMMAND) --body $< > $@
