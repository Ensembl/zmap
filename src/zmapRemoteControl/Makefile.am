
include $(top_srcdir)/Makefile.zmap

SUBDIRS = remotetest

noinst_LTLIBRARIES = libZMapRemoteControl.la

libZMapRemoteControl_la_SOURCES = \
zmapRemoteCommand.c \
zmapRemoteControl.c \
zmapRemoteControl_P.h \
$(NULL)

libZMapRemoteControl_la_CFLAGS = $(INCLUDE_COMMON)


# Extra files to remove for the maintainer-clean target.
#
MAINTAINERCLEANFILES = $(top_srcdir)/zmapRemoteControl/Makefile.in

