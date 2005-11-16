
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

libZMapXML_a_SOURCES = $(top_srcdir)/zmapXML/zmapXML_P.h \
	$(top_srcdir)/zmapXML/zmapXMLDocument.c \
	$(top_srcdir)/zmapXML/zmapXMLElement.c \
	$(top_srcdir)/zmapXML/zmapXMLAttribute.c \
	$(top_srcdir)/zmapXML/zmapXMLParse.c \
	$(top_srcdir)/zmapXML/zmapXMLFactory.c

libZMapApp_a_SOURCES = $(top_srcdir)/zmapApp/zmapApp_P.h \
	$(top_srcdir)/zmapApp/zmapAppconnect.c \
	$(top_srcdir)/zmapApp/zmapAppmain_c.c \
	$(top_srcdir)/zmapApp/zmapAppmanage.c \
	$(top_srcdir)/zmapApp/zmapAppmenubar.c \
	$(top_srcdir)/zmapApp/zmapAppwindow.c \
	$(top_srcdir)/zmapApp/zmapAppremote.c \
	$(top_srcdir)/zmapManager/zmapManager.c \
	$(top_srcdir)/zmapManager/zmapManager_P.h

libZMap_a_SOURCES = $(top_srcdir)/zmapConfig/zmapConfig.c \
	$(top_srcdir)/zmapConfig/zmapConfigGet.c \
	$(top_srcdir)/zmapConfig/zmapConfigRead.c \
	$(top_srcdir)/zmapConfig/zmapConfigUtils.c \
	$(top_srcdir)/zmapConfig/zmapConfigWrite.c \
	$(top_srcdir)/zmapConfig/zmapConfig_P.h \
	$(top_srcdir)/zmapControl/zmapControl.c \
	$(top_srcdir)/zmapControl/zmapControlNavigator.c \
	$(top_srcdir)/zmapControl/zmapControlRemote.c \
	$(top_srcdir)/zmapControl/zmapControlViews.c \
	$(top_srcdir)/zmapControl/zmapControlWindow.c \
	$(top_srcdir)/zmapControl/zmapControlWindowButtons.c \
	$(top_srcdir)/zmapControl/zmapControlWindowFrame.c \
	$(top_srcdir)/zmapControl/zmapControlWindowMenubar.c \
	$(top_srcdir)/zmapControl/zmapControl_P.h \
	$(top_srcdir)/zmapControl/remote/zmapXRemote_P.h \
	$(top_srcdir)/zmapControl/remote/zmapXRemote.c \
	$(top_srcdir)/zmapControl/zmapNavigator_P.h \
	$(top_srcdir)/zmapDocs/ZMapConfigFiles.c \
	$(top_srcdir)/zmapDocs/ZMapMainPage.c \
	$(top_srcdir)/zmapDraw/zmapDraw.c \
	$(top_srcdir)/zmapDraw/zmapDraw_P.h \
	$(top_srcdir)/zmapFeature/zmapFeature.c \
	$(top_srcdir)/zmapFeature/zmapFeatureTypes.c \
	$(top_srcdir)/zmapFeature/zmapFeatureUtils.c \
	$(top_srcdir)/zmapFeature/zmapFeatureFormatInput.c \
	$(top_srcdir)/zmapFeature/zmapFeature_P.h \
	$(top_srcdir)/zmapGFF/zmapGFF2parser.c \
	$(top_srcdir)/zmapGFF/zmapGFF_P.h \
	$(top_srcdir)/zmapUtils/zmapCmdLineArgs.c \
	$(top_srcdir)/zmapUtils/zmapCmdLineArgs_P.h \
	$(top_srcdir)/zmapUtils/zmapConfigDir.c \
	$(top_srcdir)/zmapUtils/zmapConfigDir_P.h \
	$(top_srcdir)/zmapUtils/zmapFileUtils.c \
	$(top_srcdir)/zmapUtils/zmapDNAUtils.c \
	$(top_srcdir)/zmapUtils/zmapGUIutils.c \
	$(top_srcdir)/zmapUtils/zmapGLibUtils.c \
	$(top_srcdir)/zmapUtils/zmapLogging.c \
	$(top_srcdir)/zmapUtils/zmapReadLine.c \
	$(top_srcdir)/zmapUtils/zmapReadLine_P.h \
	$(top_srcdir)/zmapUtils/zmapUtils.c \
	$(top_srcdir)/zmapUtils/zmapUrl.c \
	$(top_srcdir)/zmapUtils/zmapUrlUtils.c \
	$(top_srcdir)/zmapUtils/safe-ctype.c \
	$(top_srcdir)/zmapUtils/zmapUtilsConfig.c \
	$(top_srcdir)/zmapUtils/zmapUtils_P.h \
	$(top_srcdir)/zmapView/zmapView.c \
	$(top_srcdir)/zmapView/zmapViewUtils.c \
	$(top_srcdir)/zmapView/zmapView_P.h \
	$(top_srcdir)/zmapWindow/GFF_scaffold.c \
	$(top_srcdir)/zmapWindow/zmapWindow.c \
	$(top_srcdir)/zmapWindow/zmapWindow_P.h \
	$(top_srcdir)/zmapWindow/zmapWindowCallBlixem.c \
	$(top_srcdir)/zmapWindow/zmapWindowDrawFeatures.c \
	$(top_srcdir)/zmapWindow/zmapWindowDrawFeatures_P.h \
	$(top_srcdir)/zmapWindow/zmapWindowEditor.c \
	$(top_srcdir)/zmapWindow/zmapWindowFrame.c \
	$(top_srcdir)/zmapWindow/zmapWindowItemHash.c \
	$(top_srcdir)/zmapWindow/zmapWindowList.c \
	$(top_srcdir)/zmapWindow/zmapWindowFeatureList.c \
	$(top_srcdir)/zmapWindow/zmapWindowSearch.c \
	$(top_srcdir)/zmapWindow/zmapWindowMenus.c \
	$(top_srcdir)/zmapWindow/zmapWindowUtils.c \
	$(top_srcdir)/zmapWindow/zmapWindowItem.c \
	$(top_srcdir)/zmapWindow/zmapWindowDraw.c \
	$(top_srcdir)/zmapWindow/zmapWindowZoomControl.c \
	$(top_srcdir)/zmapWindow/zmapWindowZoomControl_P.h

libZMapThr_a_SOURCES = $(top_srcdir)/zmapServer/zmapServer.c \
	$(top_srcdir)/zmapServer/zmapServerProtocolHandler.c \
	$(top_srcdir)/zmapServer/zmapServerPrototype.h \
	$(top_srcdir)/zmapServer/zmapServer_P.h \
	$(top_srcdir)/zmapServer/acedb/acedbServer.c \
	$(top_srcdir)/zmapServer/acedb/acedbServer_P.h \
	$(top_srcdir)/zmapServer/das/dasServer.c \
	$(top_srcdir)/zmapServer/das/dasServer_P.h \
	$(top_srcdir)/zmapServer/das/das1schema.h \
	$(top_srcdir)/zmapServer/das/das1meta.c \
	$(top_srcdir)/zmapServer/das/das1features.c \
	$(top_srcdir)/zmapServer/das/das1XMLhandlers.c \
	$(top_srcdir)/zmapServer/file/fileServer.c \
	$(top_srcdir)/zmapServer/file/fileServer_P.h \
	$(top_srcdir)/zmapThreads/zmapSlave.c \
	$(top_srcdir)/zmapThreads/zmapSlave_P.h \
	$(top_srcdir)/zmapThreads/zmapThreads.c \
	$(top_srcdir)/zmapThreads/zmapThreadsUtils.c \
	$(top_srcdir)/zmapThreads/zmapThreads_P.h

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

libZMapApp_a_DEPENDENCIES = src-link
libZMap_a_DEPENDENCIES = src-link libZMapXML.a
libZMapThr_a_DEPENDENCIES = src-link libZMapXML.a


src-link: 
	echo " ---------- INTENTIONALLY BLANK $@"
##	@test -L $(builddir)/$@ || ln -s `pwd`/$@ $(BINDIR)

forcemake: 
