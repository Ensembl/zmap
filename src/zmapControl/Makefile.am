
include $(top_srcdir)/Makefile.common

SUBDIRS = .

noinst_LTLIBRARIES = libZMapControl.la

libZMapControl_la_SOURCES = zmapControl.c \
zmapControlImportFile.c\
zmapControlNavigator.c \
zmapControlPreferences.c \
zmapControlRemoteControl.c \
zmapControlViews.c \
zmapControlWindow.c \
zmapControlWindowButtons.c \
zmapControlWindowFrame.c \
zmapControlWindowInfoPanel.c \
zmapControlWindowMenubar.c \
zmapControl_P.h \
zmapNavigator_P.h \
$(NULL)

libZMapControl_la_CFLAGS = $(INCLUDE_COMMON)




# Extra files to remove for the maintainer-clean target.
#
MAINTAINERCLEANFILES = $(top_srcdir)/zmapControl/Makefile.in
