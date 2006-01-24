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
	bootstrap \
	$(DX_CONFIG)
	$(NULL)

# support for pkg-config
pkgconfigdir   = $(libdir)/pkgconfig
pkgconfig_DATA = zmap.pc

################################################################################
################################################################################
# Building zmap
################################################################################
################################################################################

bin_PROGRAMS = zmap gffparser

# should this be zmap_OBJECTS ??????
zmap_SOURCES = $(top_srcdir)/zmapApp/zmapApp_P.h 
#	zmapApp/zmapApp_P.h \
#	zmapApp/zmapAppwindow.c \
#	zmapApp/zmapAppmenubar.c \
#	zmapApp/zmapAppmanage.c \
#	zmapApp/zmapAppconnect.c \
#	$(NULL) 

# -lZMap was at the end, why is this?
zmap_LDFLAGS = -L$(top_builddir)/lib -lZMapApp -lZMap -lZMapThr -lZMap -lZMapXML -lexpat -lpopt \
	@ACECONN_LIBS@ \
	@GTK_LIBS@ \
	@FOOCANVAS_LIBS@ \
	@LIBCURL@ \
	@X_PRE_LIBS@ \
        @X_LIBS@ \
	$(NULL)

zmap_CPPFLAGS = $(AM_CPPFLAGS) -I$(top_srcdir)/zmapApp

zmap_DEPENDENCIES = $(LIBS_ZMAP) src-copy

# DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc

gffparser_SOURCES = $(top_srcdir)/zmapGFF/gffparser.c
gffparser_LDFLAGS = -L$(top_builddir)/lib \
	@GTK_LIBS@ \
	@FOOCANVAS_LIBS@ \
	@LIBCURL@ \
	@X_PRE_LIBS@ \
        @X_LIBS@ \
	$(NULL)
gffparser_LDADD        = -lZMap
gffparser_DEPENDENCIES = $(LIBS_ZMAP)
gffparser_CPPFLAGS     = $(AM_CPPFLAGS) -I$(top_srcdir)/zmapApp


################################################################################
################################################################################
# Extra targets
################################################################################
################################################################################

mkdocdir:
	@if test ! -d docs ; then \
	  mkdir docs; \
	fi

docs: mkdocdir doxygen-doc

maintainer-clean-local:
	rm -rf cleanDir

src-copy: 
	@echo This is src-copy
