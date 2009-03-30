
include $(top_srcdir)/Makefile.common

SUBDIRS = .

noinst_LTLIBRARIES = libZMapControl.la

libZMapControl_la_SOURCES = zmapControlRemoteSend.c \
zmapNavigator_P.h \
zmapControlWindowButtons.c \
zmapControlWindowFrame.c \
zmapControlWindowMenubar.c \
zmapControl.c \
zmapControlWindow.c \
zmapControlViews.c \
zmapControlWindowInfoPanel.c \
zmapControlPreferences.c \
zmapControlNavigator.c \
zmapControlRemoteReceive.c \
zmapControl_P.h \
remote/zmapXRemoteAPI.h \
remote/zmapXRemote_P.h \
remote/zmapXRemote.c \
$(NULL)

libZMapControl_la_CFLAGS = $(INCLUDE_COMMON)
