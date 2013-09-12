
include $(top_srcdir)/Makefile.common

SUBDIRS = .

noinst_LTLIBRARIES = libZMapApp.la

libZMapApp_la_SOURCES = zmapApp_P.h \
zmapAppconnect.c \
zmapAppmain_c.c \
zmapAppmanage.c \
zmapAppmenubar.c \
zmapAppwindow.c \
zmapAppRemoteControl.c \
zmapAppRemoteCommand.c \
$(NULL)

libZMapApp_la_CFLAGS = $(INCLUDE_COMMON)


# Extra files to remove for the maintainer-clean target.
#
MAINTAINERCLEANFILES = $(top_srcdir)/zmapApp/Makefile.in

