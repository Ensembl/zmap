# Process this file with automake to create Makefile.in.
#
# The Main ZMap Makefile.am
#
# binary dist with
# make install DESTDIR=/tmp/dist
# tar -C /tmp/dist -cvf package-version.tar
# gzip -9 package-version.tar
################################################################################
################################################################################
# General Housekeeping
################################################################################
################################################################################

include $(srcdir)/Makefile.common
include $(srcdir)/am_doxygen.am

# ZMAP_SRC_DIRS See configure.ac

SUBDIRS = $(ZMAP_SRC_DIRS) include lib

CLEANFILES = $(pkgconfig_DATA)

MOSTLYCLEANFILES = $(DX_CLEANFILES)

# toplevel files
# noinst_DATA = INSTALL

EXTRA_DIST = INSTALL \
	README.zmap \
	bootstrap
	$(NULL)

# support for pkg-config
pkgconfigdir   = $(libdir)/pkgconfig
pkgconfig_DATA = zmap.pc

################################################################################
################################################################################
# Building zmap
################################################################################
################################################################################

bin_PROGRAMS = zmap

# should this be zmap_OBJECTS ??????
zmap_SOURCES = $(top_srcdir)/zmapApp/zmapApp_P.h 
#	zmapApp/zmapApp_P.h \
#	zmapApp/zmapAppwindow.c \
#	zmapApp/zmapAppmenubar.c \
#	zmapApp/zmapAppmanage.c \
#	zmapApp/zmapAppconnect.c \
#	$(NULL) 

# -lZMap was at the end, why is this?
zmap_LDFLAGS = -L$(top_srcdir)/lib -lZMapApp -lZMap -lZMapThr -lZMap -lexpat -lpopt \
	$(ACECONN_LIBS) \
	$(GTK_LIBS) \
	$(FOOCANVAS_LIBS) \
	$(CURL_LIBS)
zmap_CPPFLAGS = $(AM_CPPFLAGS) -I$(top_srcdir)/zmapApp

zmap_DEPENDENCIES = $(LIBS_ZMAP) src-copy

# DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc


################################################################################
################################################################################
# Extra targets
################################################################################
################################################################################

Doxyfile:
	@echo "using doxygen configuration in $@"

docs: doxygen-doc

maintainer-clean-local:
	rm -rf cleanDir

src-copy: 
	@echo This is src-copy
