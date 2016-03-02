

include $(top_srcdir)/Makefile.common

SUBDIRS = .

noinst_LTLIBRARIES = libZMapRemoteControl.la

libZMapRemoteControl_la_SOURCES = \
remotecontroltest.cpp \
$(NULL)

libZMapRemoteControl_la_CFLAGS = $(INCLUDE_COMMON)


# Extra files to remove for the maintainer-clean target.
#
MAINTAINERCLEANFILES = $(top_srcdir)/zmapRemoteControl/remotecontrol/Makefile.in
