
include $(top_srcdir)/Makefile.common

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
zmapAppRemote_P.hpp \
zmapAppManager.cpp \
$(NULL)


if USE_ZEROMQ
libZMapApp_la_SOURCES += zmapAppRemoteInternal.cpp zmapAppRemoteInternal.hpp
endif


libZMapApp_la_CFLAGS = $(INCLUDE_COMMON)


# Extra files to remove for the maintainer-clean target.
#
MAINTAINERCLEANFILES = $(top_srcdir)/zmapApp/Makefile.in


# See the explanation in zmapConfig/Makefile.am for why these unrelated
# targets has been put here.
#
all-local:
	@echo 'about to make zmapConfigs default_styles.cpp'
	$(top_srcdir)/zmapConfig/zmap_DefaultStyles.pl $(top_srcdir)/zmapConfig/default_styles.txt > $(top_srcdir)/zmapConfig/default_styles.cpp
	@echo 'finished making zmapConfigs default_styles.cpp'

dist-hook:
	@echo 'about to make zmapConfigs default_styles.cpp'
	$(top_srcdir)/zmapConfig/zmap_DefaultStyles.pl $(top_srcdir)/zmapConfig/default_styles.txt > $(top_srcdir)/zmapConfig/default_styles.cpp
	@echo 'finished making zmapConfigs default_styles.cpp'
