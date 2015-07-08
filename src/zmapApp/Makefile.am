
include $(top_srcdir)/Makefile.zmap

SUBDIRS = .

noinst_LTLIBRARIES = libZMapApp.la

libZMapApp_la_SOURCES = zmapApp_P.h \
zmapAppconnect.c \
zmapAppmain_c.cc \
zmapAppmanagepanel.c \
zmapAppmenubar.c \
zmapAppwindow.c \
zmapAppRemoteControl.c \
zmapAppRemoteCommand.c \
zmapAppManager.c \
$(NULL)


if USE_ZEROMQ
libZMapApp_la_SOURCES += zmapAppRemoteInternal.c zmapAppRemoteInternal.h
endif


libZMapApp_la_CFLAGS = $(INCLUDE_COMMON)


# Extra files to remove for the maintainer-clean target.
#
MAINTAINERCLEANFILES = $(top_srcdir)/zmapApp/Makefile.in

