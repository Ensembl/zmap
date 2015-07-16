
include $(top_srcdir)/Makefile.zmap

SUBDIRS = .

noinst_LTLIBRARIES = libZMapControl.la

libZMapControl_la_SOURCES = zmapControl.cpp \
zmapControlImportFile.cpp\
zmapControlNavigator.cpp \
zmapControlPreferences.cpp \
zmapControlRemoteControl.cpp \
zmapControlViews.cpp \
zmapControlWindow.cpp \
zmapControlWindowButtons.cpp \
zmapControlWindowFrame.cpp \
zmapControlWindowInfoPanel.cpp \
zmapControlWindowMenubar.cpp \
zmapControl_P.hpp \
zmapNavigator_P.hpp \
$(NULL)

libZMapControl_la_CFLAGS = $(INCLUDE_COMMON)




# Extra files to remove for the maintainer-clean target.
#
MAINTAINERCLEANFILES = $(top_srcdir)/zmapControl/Makefile.in
