
include $(top_srcdir)/Makefile.common

noinst_LTLIBRARIES = libZMapRemoteControl.la

libZMapRemoteControl_la_SOURCES = \
zmapRemoteControl_P.hpp \
zmapRemoteCommand.cpp \
zmapRemoteControlUtils.cpp \
$(NULL)

if USE_ZEROMQ
libZMapRemoteControl_la_SOURCES += zmapRemoteControl.cpp
endif

libZMapRemoteControl_la_CFLAGS = $(INCLUDE_COMMON)


if USE_ZEROMQ
SUBDIRS = remotetest
endif


# Extra files to remove for the maintainer-clean target.
#
MAINTAINERCLEANFILES = $(top_srcdir)/zmapRemoteControl/Makefile.in

