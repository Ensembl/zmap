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

SUBDIRS = $(ZMAP_SRC_DIRS) include lib $(CANVAS_TESTING)

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

bin_PROGRAMS = zmap gffparser xml_writer xremote_gui

# should this be zmap_OBJECTS ??????
zmap_SOURCES = $(top_srcdir)/zmapApp/zmapApp_P.h 
#	zmapApp/zmapApp_P.h \
#	zmapApp/zmapAppwindow.c \
#	zmapApp/zmapAppmenubar.c \
#	zmapApp/zmapAppmanage.c \
#	zmapApp/zmapAppconnect.c \
#	$(NULL) 

# -lZMap was at the end, why is this?
zmap_LDFLAGS = -L$(top_builddir)/lib \
	-lZMapApp -lZMap -lZMapThr -lZMap -lZMapXML \
	-lpfetch -lcurlobj -lexpat -lpopt -lg2 -lgd \
	@ACECONN_LIBS@ \
	@GTK_LIBS@ \
	@FOOCANVAS_LIBS@ \
	@LIBCURL@ \
	@X_PRE_LIBS@ \
        @X_LIBS@ \
	@MACOSX_UNIVERSAL_FLAGS@ @MACOSX_UNIVERSAL_FILE@ $(NULL)

zmap_CPPFLAGS = $(AM_CPPFLAGS) -I$(top_srcdir)/zmapApp

zmap_DEPENDENCIES = $(LIBS_ZMAP) @MACOSX_UNIVERSAL_FILE@

# DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc

gffparser_SOURCES = $(top_srcdir)/zmapGFF/gffparser.c
gffparser_LDFLAGS = -L$(top_builddir)/lib \
	@GTK_LIBS@ \
	@FOOCANVAS_LIBS@ \
	@LIBCURL@ \
	@X_PRE_LIBS@ \
        @X_LIBS@ \
	@MACOSX_UNIVERSAL_FLAGS@ $(NULL)
gffparser_LDADD        = -lZMap
gffparser_DEPENDENCIES = $(LIBS_ZMAP)
gffparser_CPPFLAGS     = $(AM_CPPFLAGS) -I$(top_srcdir)/zmapApp

xml_writer_SOURCES = $(top_srcdir)/zmapXML/example_xml_writer.c
xml_writer_LDFLAGS = -L$(top_builddir)/lib \
	@GTK_LIBS@ \
	@FOOCANVAS_LIBS@ \
	@LIBCURL@ \
	@X_PRE_LIBS@ \
        @X_LIBS@ \
	@MACOSX_UNIVERSAL_FLAGS@ $(NULL)
xml_writer_LDADD        = -lZMap -lZMapXML
xml_writer_DEPENDENCIES = $(LIBS_ZMAP)
xml_writer_CPPFLAGS     = $(AM_CPPFLAGS) -I$(top_srcdir)/zmapApp


xremote_gui_SOURCES = $(top_srcdir)/zmapControl/remote/xremote_gui_test.c
xremote_gui_LDFLAGS = -L$(top_builddir)/lib -lZMapXML -lexpat -lpopt -lg2 -lgd \
	@GTK_LIBS@ \
	@FOOCANVAS_LIBS@ \
	@LIBCURL@ \
	@X_PRE_LIBS@ \
        @X_LIBS@ \
	@MACOSX_UNIVERSAL_FLAGS@ $(NULL)
xremote_gui_LDADD        = -lZMap -lZMapXML
xremote_gui_DEPENDENCIES = $(LIBS_ZMAP)
xremote_gui_CPPFLAGS     = $(AM_CPPFLAGS) -I$(top_srcdir)/zmapApp


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

# the next two targets are required for installing the static
# libraries.  Not something we should be doing I'm sure, but
# rebuilding the xremote perl code requires them.
install-data-local: install-binPROGRAMS install-zmap-static-libs

install-zmap-static-libs: 
	@$(NORMAL_INSTALL)
	@echo "************************************************************"
	@echo "* Installing $@"
	test -z "$(libdir)" || $(mkdir_p) "$(DESTDIR)$(libdir)"
	@list='$(LIBS_ZMAP)'; for p in $$list; do \
	  if test -f "$$p"; then d=; else d="$(srcdir)/"; fi; \
	  f=$(am__strip_dir) \
	  echo " $(INSTALL_DATA) '$$d$$p' '$(DESTDIR)/$(libdir)/$$f'"; \
	  $(INSTALL_DATA) "$$d$$p" "$(DESTDIR)/$(libdir)/$$f"; \
	done
	@echo "************************************************************"

@MACOSX_UNIVERSAL_FILE@:
	touch @MACOSX_UNIVERSAL_FILE@
