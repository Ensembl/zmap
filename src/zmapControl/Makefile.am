
include $(top_srcdir)/Makefile.common

SUBDIRS = .

noinst_LTLIBRARIES = libZMapControl.la

libZMapControl_la_SOURCES = zmapControl.c \
zmapControlImportFile.c\
zmapControlNavigator.c \
zmapControlPreferences.c \
zmapControlRemoteReceive.c \
zmapControlRemoteSend.c \
zmapControlViews.c \
zmapControlWindow.c \
zmapControlWindowButtons.c \
zmapControlWindowFrame.c \
zmapControlWindowInfoPanel.c \
zmapControlWindowMenubar.c \
zmapControl_P.h \
zmapNavigator_P.h \
remote/zmapXRemote.c \
remote/zmapXRemoteAPI.h \
remote/zmapXRemote_P.h \
$(NULL)

libZMapControl_la_CFLAGS = $(INCLUDE_COMMON)
