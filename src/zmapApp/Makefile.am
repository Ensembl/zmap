
include $(top_srcdir)/Makefile.zmap

SUBDIRS = .

noinst_LTLIBRARIES = libZMapApp.la

libZMapApp_la_SOURCES = zmapApp_P.hpp \
zmapAppconnect.cpp \
zmapAppmain_c.cpp \
zmapAppmanagepanel.cpp \
zmapAppmenubar.cpp \
zmapAppwindow.cpp \
zmapAppRemoteInternal.hpp \
zmapAppRemoteInternal.cpp \
zmapAppRemoteControl.cpp \
zmapAppRemoteCommand.cpp \
zmapAppManager.cpp \
$(NULL)


if USE_ZEROMQ
libZMapApp_la_SOURCES += zmapAppRemoteInternal.cpp zmapAppRemoteInternal.hpp
endif


libZMapApp_la_CFLAGS = $(INCLUDE_COMMON)


# Extra files to remove for the maintainer-clean target.
#
MAINTAINERCLEANFILES = $(top_srcdir)/zmapApp/Makefile.in

