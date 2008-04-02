
include $(top_srcdir)/Makefile.common
################################################################################
# Building Libraries
################################################################################

LIBS_ZMAP_LOCAL = libZMapApp.a \
	libZMap.a \
	libZMapThr.a \
        libZMapXML.a \
	$(NULL)

noinst_LIBRARIES = $(LIBS_ZMAP_LOCAL)


libZMapXML_a_SOURCES = $(top_srcdir)/zmapXML/zmapXMLAttribute.c \
$(top_srcdir)/zmapXML/zmapXMLDocument.c \
$(top_srcdir)/zmapXML/zmapXMLElement.c \
$(top_srcdir)/zmapXML/zmapXMLParse.c \
$(top_srcdir)/zmapXML/zmapXMLUtils.c \
$(top_srcdir)/zmapXML/zmapXMLWriter.c \
$(top_srcdir)/zmapXML/zmapXML_P.h \
$(NULL)

libZMapApp_a_SOURCES = $(top_srcdir)/zmapApp/zmapApp_P.h \
$(top_srcdir)/zmapApp/zmapAppconnect.c \
$(top_srcdir)/zmapApp/zmapAppmain_c.c \
$(top_srcdir)/zmapApp/zmapAppmanage.c \
$(top_srcdir)/zmapApp/zmapAppmenubar.c \
$(top_srcdir)/zmapApp/zmapAppremote.c \
$(top_srcdir)/zmapApp/zmapAppwindow.c \
$(top_srcdir)/zmapManager/zmapManager.c \
$(top_srcdir)/zmapManager/zmapManager_P.h \
$(NULL)

libZMap_a_SOURCES = $(top_srcdir)/zmapConfig/zmapConfig.c \
$(top_srcdir)/zmapConfig/zmapConfigGet.c \
$(top_srcdir)/zmapConfig/zmapConfigRead.c \
$(top_srcdir)/zmapConfig/zmapConfigUtils.c \
$(top_srcdir)/zmapConfig/zmapConfigWrite.c \
$(top_srcdir)/zmapConfig/zmapConfig_P.h \
$(top_srcdir)/zmapControl/remote/zmapXRemote.c \
$(top_srcdir)/zmapControl/remote/zmapXRemote_P.h \
$(top_srcdir)/zmapControl/remote/zmap_crash_handler.c \
$(top_srcdir)/zmapControl/zmapControl.c \
$(top_srcdir)/zmapControl/zmapControlNavigator.c \
$(top_srcdir)/zmapControl/zmapControlPreferences.c \
$(top_srcdir)/zmapControl/zmapControlRemoteReceive.c \
$(top_srcdir)/zmapControl/zmapControlRemoteSend.c \
$(top_srcdir)/zmapControl/zmapControlViews.c \
$(top_srcdir)/zmapControl/zmapControlWindow.c \
$(top_srcdir)/zmapControl/zmapControlWindowButtons.c \
$(top_srcdir)/zmapControl/zmapControlWindowFrame.c \
$(top_srcdir)/zmapControl/zmapControlWindowInfoPanel.c \
$(top_srcdir)/zmapControl/zmapControlWindowMenubar.c \
$(top_srcdir)/zmapControl/zmapControl_P.h \
$(top_srcdir)/zmapControl/zmapNavigator_P.h \
$(top_srcdir)/zmapDAS/zmapDAS1DSN.c \
$(top_srcdir)/zmapDAS/zmapDAS1Parser.c \
$(top_srcdir)/zmapDocs/ZMapMainPage.c \
$(top_srcdir)/zmapDraw/zmapDraw.c \
$(top_srcdir)/zmapDraw/zmapDraw_P.h \
$(top_srcdir)/zmapFeature/zmapFeature.c \
$(top_srcdir)/zmapFeature/zmapFeatureContext.c \
$(top_srcdir)/zmapFeature/zmapFeatureContextUtils.c \
$(top_srcdir)/zmapFeature/zmapFeatureFormatInput.c \
$(top_srcdir)/zmapFeature/zmapFeatureTypes.c \
$(top_srcdir)/zmapFeature/zmapFeatureUtils.c \
$(top_srcdir)/zmapFeature/zmapFeatureXML.c \
$(top_srcdir)/zmapFeature/zmapFeature_P.h \
$(top_srcdir)/zmapFeature/zmapStyle.c \
$(top_srcdir)/zmapFeature/zmapStyle_P.h \
$(top_srcdir)/zmapGFF/zmapGFF2Dumper.c \
$(top_srcdir)/zmapGFF/zmapGFF2parser.c \
$(top_srcdir)/zmapGFF/zmapGFF_P.h \
$(top_srcdir)/zmapUtils/safe-ctype.c \
$(top_srcdir)/zmapUtils/safe-ctype.h \
$(top_srcdir)/zmapUtils/zmapCmdLineArgs.c \
$(top_srcdir)/zmapUtils/zmapCmdLineArgs_P.h \
$(top_srcdir)/zmapUtils/zmapConfigDir.c \
$(top_srcdir)/zmapUtils/zmapConfigDir_P.h \
$(top_srcdir)/zmapUtils/zmapDNA.c \
$(top_srcdir)/zmapUtils/zmapFASTA.c \
$(top_srcdir)/zmapUtils/zmapFileUtils.c \
$(top_srcdir)/zmapUtils/zmapFooUtils.c \
$(top_srcdir)/zmapUtils/zmapGLibUtils.c \
$(top_srcdir)/zmapUtils/zmapGUINotebook.c \
$(top_srcdir)/zmapUtils/zmapGUImenus.c \
$(top_srcdir)/zmapUtils/zmapGUIutils.c \
$(top_srcdir)/zmapUtils/zmapHandle.c \
$(top_srcdir)/zmapUtils/zmapIOOut.c \
$(top_srcdir)/zmapUtils/zmapIO_P.h \
$(top_srcdir)/zmapUtils/zmapLogging.c \
$(top_srcdir)/zmapUtils/zmapPeptide.c \
$(top_srcdir)/zmapUtils/zmapReadLine.c \
$(top_srcdir)/zmapUtils/zmapReadLine_P.h \
$(top_srcdir)/zmapUtils/zmapSequence.c \
$(top_srcdir)/zmapUtils/zmapString.c \
$(top_srcdir)/zmapUtils/zmapUrl.c \
$(top_srcdir)/zmapUtils/zmapUrlOptions.h \
$(top_srcdir)/zmapUtils/zmapUrlUtils.c \
$(top_srcdir)/zmapUtils/zmapUrlUtils.h \
$(top_srcdir)/zmapUtils/zmapUtils.c \
$(top_srcdir)/zmapUtils/zmapUtilsConfig.c \
$(top_srcdir)/zmapUtils/zmapUtilsSpawn.c \
$(top_srcdir)/zmapUtils/zmapUtils_P.h \
$(top_srcdir)/zmapUtils/zmapWebBrowser.c \
$(top_srcdir)/zmapUtils/zmapXRemoteUtils.c \
$(top_srcdir)/zmapView/zmapView.c \
$(top_srcdir)/zmapView/zmapViewCallBlixem.c \
$(top_srcdir)/zmapView/zmapViewRemoteReceive.c \
$(top_srcdir)/zmapView/zmapViewRemoteSend.c \
$(top_srcdir)/zmapView/zmapViewUtils.c \
$(top_srcdir)/zmapView/zmapView_P.h \
$(top_srcdir)/zmapWindow/zmapWindow.c \
$(top_srcdir)/zmapWindow/zmapWindowColBump.c \
$(top_srcdir)/zmapWindow/zmapWindowColConfig.c \
$(top_srcdir)/zmapWindow/zmapWindowColOrder.c \
$(top_srcdir)/zmapWindow/zmapWindowContainer.c \
$(top_srcdir)/zmapWindow/zmapWindowContainer.h \
$(top_srcdir)/zmapWindow/zmapWindowDNA.c \
$(top_srcdir)/zmapWindow/zmapWindowDNAChoose.c \
$(top_srcdir)/zmapWindow/zmapWindowDNAList.c \
$(top_srcdir)/zmapWindow/zmapWindowDraw.c \
$(top_srcdir)/zmapWindow/zmapWindowDrawFeatures.c \
$(top_srcdir)/zmapWindow/zmapWindowDump.c \
$(top_srcdir)/zmapWindow/zmapWindowExport.c \
$(top_srcdir)/zmapWindow/zmapWindowFeature.c \
$(top_srcdir)/zmapWindow/zmapWindowFeatureList.c \
$(top_srcdir)/zmapWindow/zmapWindowFeatureShow.c \
$(top_srcdir)/zmapWindow/zmapWindowFocus.c \
$(top_srcdir)/zmapWindow/zmapWindowFrame.c \
$(top_srcdir)/zmapWindow/zmapWindowItem.c \
$(top_srcdir)/zmapWindow/zmapWindowItemDebug.c \
$(top_srcdir)/zmapWindow/zmapWindowItemFactory.c \
$(top_srcdir)/zmapWindow/zmapWindowItemFactory.h \
$(top_srcdir)/zmapWindow/zmapWindowItemFeatureSet.c \
$(top_srcdir)/zmapWindow/zmapWindowItemHash.c \
$(top_srcdir)/zmapWindow/zmapWindowItemText.c \
$(top_srcdir)/zmapWindow/zmapWindowItemTextFillColumn.h \
$(top_srcdir)/zmapWindow/zmapWindowList.c \
$(top_srcdir)/zmapWindow/zmapWindowLongItems.c \
$(top_srcdir)/zmapWindow/zmapWindowMark.c \
$(top_srcdir)/zmapWindow/zmapWindowMenus.c \
$(top_srcdir)/zmapWindow/zmapWindowNavigator.c \
$(top_srcdir)/zmapWindow/zmapWindowNavigatorMenus.c \
$(top_srcdir)/zmapWindow/zmapWindowNavigatorWidget.c \
$(top_srcdir)/zmapWindow/zmapWindowNavigator_P.h \
$(top_srcdir)/zmapWindow/zmapWindowOverlays.c \
$(top_srcdir)/zmapWindow/zmapWindowOverlays.h \
$(top_srcdir)/zmapWindow/zmapWindowPrint.c \
$(top_srcdir)/zmapWindow/zmapWindowRemoteReceive.c \
$(top_srcdir)/zmapWindow/zmapWindowRuler.c \
$(top_srcdir)/zmapWindow/zmapWindowSearch.c \
$(top_srcdir)/zmapWindow/zmapWindowState.c \
$(top_srcdir)/zmapWindow/zmapWindowState.h \
$(top_srcdir)/zmapWindow/zmapWindowStats.c \
$(top_srcdir)/zmapWindow/zmapWindowTextPositioner.c \
$(top_srcdir)/zmapWindow/zmapWindowTextPositioner.h \
$(top_srcdir)/zmapWindow/zmapWindowUtils.c \
$(top_srcdir)/zmapWindow/zmapWindowZoomControl.c \
$(top_srcdir)/zmapWindow/zmapWindowZoomControl_P.h \
$(top_srcdir)/zmapWindow/zmapWindow_P.h \
$(NULL)

libZMapThr_a_SOURCES = $(top_srcdir)/zmapServer/acedb/acedbServer.c \
$(top_srcdir)/zmapServer/acedb/acedbServer_P.h \
$(top_srcdir)/zmapServer/zmapServer.c \
$(top_srcdir)/zmapServer/zmapServer.h \
$(top_srcdir)/zmapServer/zmapServerProtocolHandler.c \
$(top_srcdir)/zmapServer/zmapServerPrototype.h \
$(top_srcdir)/zmapServer/das/dasServer.c \
$(top_srcdir)/zmapServer/das/dasServer_P.h \
$(top_srcdir)/zmapServer/file/fileServer.c \
$(top_srcdir)/zmapServer/file/fileServer_P.h \
$(top_srcdir)/zmapServer/zmapServer_P.h \
$(top_srcdir)/zmapThreads/zmapSlave.c \
$(top_srcdir)/zmapThreads/zmapSlave_P.h \
$(top_srcdir)/zmapThreads/zmapThreads.c \
$(top_srcdir)/zmapThreads/zmapThreadsUtils.c \
$(top_srcdir)/zmapThreads/zmapThreads_P.h \
$(NULL)



libZMapXML_a_CPPFLAGS = $(AM_CPPFLAGS) \
	-I$(top_srcdir)/zmapXML

libZMapApp_a_CPPFLAGS = $(AM_CPPFLAGS) \
	-I$(top_srcdir)/zmapApp \
	-I$(top_srcdir)/zmapManager
libZMap_a_CPPFLAGS = $(AM_CPPFLAGS) \
	-I$(top_srcdir)/zmapConfig \
	-I$(top_srcdir)/zmapControl \
	-I$(top_srcdir)/zmapDocs \
	-I$(top_srcdir)/zmapDraw \
	-I$(top_srcdir)/zmapFeature \
	-I$(top_srcdir)/zmapGFF \
	-I$(top_srcdir)/zmapUtils \
	-I$(top_srcdir)/zmapView \
	-I$(top_srcdir)/zmapWindow
libZMapThr_a_CPPFLAGS = $(AM_CPPFLAGS) \
	-I$(top_srcdir)/zmapServer \
	-I$(top_srcdir)/zmapServer/acedb \
	-I$(top_srcdir)/zmapServer/das \
	-I$(top_srcdir)/zmapServer/file \
	-I$(top_srcdir)/zmapThreads

libZMapApp_a_DEPENDENCIES = 
libZMap_a_DEPENDENCIES = libZMapXML.a
libZMapThr_a_DEPENDENCIES = libZMapXML.a


forcemake: 

