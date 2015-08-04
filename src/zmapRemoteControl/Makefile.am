
include $(top_srcdir)/Makefile.zmap

noinst_LTLIBRARIES = libZMapRemoteControl.la

libZMapRemoteControl_la_SOURCES = \
zmapRemoteControl_P.h \
zmapRemoteCommand.c \
zmapRemoteControlUtils.c \
$(NULL)

if USE_ZEROMQ
libZMapRemoteControl_la_SOURCES += zmapRemoteControl.c
endif

libZMapRemoteControl_la_CFLAGS = $(INCLUDE_COMMON)


if USE_ZEROMQ
SUBDIRS = remotetest
endif


# Extra files to remove for the maintainer-clean target.
#
MAINTAINERCLEANFILES = $(top_srcdir)/zmapRemoteControl/Makefile.in

