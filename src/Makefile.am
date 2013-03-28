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


# reinclude this once I know how to do it conditionally....
#include $(srcdir)/am_doxygen.am


# Local macros.
#
ACLOCAL_AMFLAGS = -I m4




# ZMAP_SRC_DIRS See configure.ac

SUBDIRS = AceConn libpfetch libcurlobject libzmapfoocanvas include $(ZMAP_SRC_DIRS) $(CANVAS_TESTING)

CLEANFILES = $(pkgconfig_DATA)

MOSTLYCLEANFILES = $(DX_CLEANFILES)




# toplevel files
# noinst_DATA = INSTALL


EXTRA_DIST = INSTALL \
	README.zmap \
	$(NULL)


# support for pkg-config
pkgconfigdir   = $(libdir)/pkgconfig
pkgconfig_DATA = zmap.pc


################################################################################
################################################################################
# Building zmap
################################################################################
################################################################################


#
# Libraries: code is compiled into a series of libXXX.a files, in some cases
# some of these are further combined into 'parent' libXXX.a files.
#

noinst_LTLIBRARIES = libZMapApp.la libZMap.la libZMapAppServices.la \
                     libZMapThr.la libZMapRemoteControl.la libZMapXML.la libZMapUtils.la \
                     libzmapfoocanvas.la libPFetch.la

libZMapApp_la_SOURCES =
libZMapApp_la_LIBADD  = zmapApp/libZMapApp.la

libZMap_la_SOURCES =
libZMap_la_LIBADD  = \
zmapControl/libZMapControl.la \
zmapConfig/libZMapConfig.la   \
zmapDAS/libZMapDas.la         \
zmapDraw/libZMapDraw.la       \
zmapFeature/libZMapFeature.la \
zmapGFF/libZMapGFF.la         \
zmapManager/libZMapManager.la \
zmapView/libZMapView.la       \
zmapWindow/libZMapWindow.la   \
zmapServer/libZMapServer.la   \
zmapServer/libZMapServers.la  \
$(NULL)

libZMapAppServices_la_SOURCES = 
libZMapAppServices_la_LIBADD  = zmapAppServices/libZMapAppServices.la

libZMapThr_la_SOURCES =
libZMapThr_la_LIBADD  =       \
zmapThreads/libZMapThreads.la \
$(NULL)

libZMapRemoteControl_la_SOURCES = 
libZMapRemoteControl_la_LIBADD  = zmapRemoteControl/libZMapRemoteControl.la

libZMapXML_la_SOURCES = 
libZMapXML_la_LIBADD  = zmapXML/libZMapXML.la

libZMapUtils_la_SOURCES = 
libZMapUtils_la_LIBADD  = zmapUtils/libZMapUtils.la

libPFetch_la_SOURCES = 
libPFetch_la_LIBADD  = libpfetch/libpfetch.la libcurlobject/libcurlobject.la

libzmapfoocanvas_la_SOURCES =
libzmapfoocanvas_la_LIBADD  = libzmapfoocanvas/libzmapfoocanvas.la


# Programs we're going to compile.

#bin_PROGRAMS = zmap gffparser xml_writer xremote_gui remotecontrol
bin_PROGRAMS = zmap gffparser xml_writer remotecontrol


# I am perturbed by the fact that the x libs are before the gtk libs....
#

# Search path version.....
#zmap_LDADD = $(X_PRE_LIBS) $(X_LIBS) $(X_EXTRA_LIBS)         \
#	-L$(top_builddir)/.libs                              \
#	-lZMapApp -lZMap -lZMapThr -lZMap -lZMapXML -lPFetch \
#	$(ACECONN_LIBS) $(GTK_LIBS) $(FOOCANVAS_LIBS)        \
#	$(LIBCURL) -lexpat -lg2 -lgd                         \
#	@MACOSX_UNIVERSAL_FLAGS@ $(NULL)


# Try Jeremy's new build stuff....
# Give .a files explicitly to ensure we link the right ones.
#
#zmap_LDADD =                                                 \
#	$(top_builddir)/.libs/libZMapApp.a                   \
#	$(top_builddir)/.libs/libZMap.a                      \
#	$(top_builddir)/.libs/libZMapThr.a                   \
#	$(top_builddir)/.libs/libZMap.a                      \
#	$(top_builddir)/.libs/libZMapXML.a                   \
#	$(top_builddir)/.libs/libZMapAppServices.a           \
#	$(top_builddir)/.libs/libZMapUtils.a                 \
#	$(top_builddir)/.libs/libzmapfoocanvas.a             \
#	$(top_builddir)/.libs/libPFetch.a                    \
#        $(X_PRE_LIBS) $(X_LIBS) $(X_EXTRA_LIBS)              \
#	$(ACECONN_LIBS) $(GTK_LIBS)                          \
#	$(LIBCURL) -lexpat -lg2 -lgd                         \
#	@MACOSX_UNIVERSAL_FLAGS@ $(NULL)

zmap_LDADD =                                                 \
	$(top_builddir)/.libs/libZMapApp.a                   \
	$(top_builddir)/.libs/libZMap.a                      \
	$(top_builddir)/.libs/libZMapAppServices.a           \
	$(top_builddir)/.libs/libZMapThr.a                   \
	$(top_builddir)/.libs/libZMapRemoteControl.a         \
	$(top_builddir)/.libs/libZMapXML.a                   \
	$(top_builddir)/.libs/libZMapUtils.a                 \
	$(top_builddir)/.libs/libzmapfoocanvas.a             \
	$(top_builddir)/.libs/libPFetch.a                    \
	$(top_builddir)/AceConn/src/.libs/libAceConn.a       \
        $(X_PRE_LIBS) $(X_LIBS) $(X_EXTRA_LIBS)              \
	$(GTK_LIBS)			                     \
	$(LIBCURL) -lexpat                                   \
	@MACOSX_UNIVERSAL_FLAGS@ $(NULL)

zmap_SOURCES      = $(top_srcdir)/zmapApp/zmapAppmain_c.c
zmap_LDFLAGS      = 
# We need this so as not to have libtool prune out multiple -lZMap so we can resolve our dependency cycle :(
zmap_LINK         = $(CC) $(AM_CFLAGS) $(CFLAGS) $(AM_LDFLAGS) $(LDFLAGS) -o $@
zmap_CFLAGS       = $(AM_CPPFLAGS) -I$(top_srcdir)/zmapApp
zmap_DEPENDENCIES = $(noinst_LTLIBRARIES) @MACOSX_UNIVERSAL_FILE@
# MACOSX_UNIVERSAL_FILE was also in LDFLAGS...


# gffparser - program to parse gff...
gffparser_SOURCES      = $(top_srcdir)/zmapGFF/gffparser.c
gffparser_LDFLAGS      =
gffparser_LDADD        = $(zmap_LDADD)
gffparser_DEPENDENCIES = $(noinst_LTLIBRARIES)
gffparser_CPPFLAGS     = $(AM_CPPFLAGS)

# test program to write xml...
xml_writer_SOURCES      = $(top_srcdir)/zmapXML/example_xml_writer.c
xml_writer_LDFLAGS      =
xml_writer_LDADD        = $(zmap_LDADD)
xml_writer_DEPENDENCIES = $(noinst_LTLIBRARIES)
xml_writer_CPPFLAGS     = $(AM_CPPFLAGS) -I$(top_srcdir)/zmapApp

# program that knows about the xremote protocol.
#xremote_gui_SOURCES      = $(top_srcdir)/zmapControl/remote/xremote_gui_test.c
#xremote_gui_LDFLAGS      = 
#xremote_gui_LDADD        = $(zmap_LDADD)
#xremote_gui_DEPENDENCIES = $(noinst_LTLIBRARIES)
#xremote_gui_CPPFLAGS     = $(AM_CPPFLAGS) -I$(top_srcdir)/zmapApp


# The new program that knows about the xremote protocol.
remotecontrol_SOURCES      = $(top_srcdir)/zmapRemoteControl/remotetest/remotecontroltest.c
remotecontrol_LDFLAGS      = 
remotecontrol_LDADD        = $(zmap_LDADD)
remotecontrol_DEPENDENCIES = $(noinst_LTLIBRARIES)
remotecontrol_CPPFLAGS     = $(AM_CPPFLAGS) -I$(top_srcdir)/zmapApp


#
# OH GOSH ALL THIS TEST STUFF IS CURRENTLY UNUSED AND THEREFORE USELESS.....
#
################################################################################
################################################################################
# Testing
################################################################################
################################################################################

check_LTLIBRARIES = libcheckZMap.la

libcheckZMap_la_DEPENDENCIES = $(noinst_LTLIBRARIES) $(libcheckZMap_la_LIBADD)
libcheckZMap_la_SOURCES =
#libcheckZMap_la_LDFLAGS = -all-static
libcheckZMap_la_LIBADD  = \
libpfetch/tests/libcheckPFetch.la          \
zmapFeature/tests/libcheckZMapFeature.la   \
zmapFeature/tests/libcheckZMapFeature_P.la \
zmapFeature/tests/libcheckZMapStyle.la     \
zmapFeature/tests/libcheckZMapStyle_P.la   \
zmapUtils/tests/libcheckZMapUtilsConfig.la \
$(NULL)

TESTS = check_zmap

check_PROGRAMS = check_zmap

# so that a script can do
# make check check_zmap_LOG_FILE=log_to_check.log
check_zmap_LOG_FILE     = "default.log"
check_zmap_SOURCES      = $(top_srcdir)/tests/check_zmapmain.c
check_zmap_DEPENDENCIES = $(check_LTLIBRARIES)
check_zmap_LINK         = $(zmap_LINK)
check_zmap_LDADD        = @CHECK_LIBS@ -lcheckZMap $(zmap_LDADD)
check_zmap_CFLAGS       = @CHECK_CFLAGS@ $(INCLUDE_COMMON) \
-I$(top_srcdir)/libpfetch   \
-I$(top_srcdir)/zmapFeature \
-I$(top_srcdir)/zmapUtils   \
-DCHECK_ZMAP_LOG_TO_FILE    \
-DCHECK_ZMAP_LOG=\"$(check_zmap_LOG_FILE)\"  \
$(NULL)

# DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc



#----------------------------------------------------------------------
# Our additions to some standard targets. (the XXX-local targets are
# automakes way of allowing us to add stuff to standard targets such
# as "all" etc.)
#----------------------------------------------------------------------

# The most clean you can get your tree is to use the targer maintainer-clean
# but this does not clean out everything to the state when you check out a
# copy from git...so we add some stuff to maintainer-clean to try to achieve
# this.

#GENERATED_FILES = './.deps ./.libs ./bin ./include ./lib* ./zmap* ./Makefile ./config.* dotZMap gffparser* \
#./stamp-h1 ./xml_* ./xremote* ./zmap*'


# Standard way to add _files_ to the list deleted by the maintainer-clean target.
#
MAINTAINERCLEANFILES = $(top_srcdir)/Makefile.in $(top_srcdir)/aclocal.m4 \
$(top_srcdir)/config.h.in $(top_srcdir)/configure $(top_srcdir)/zmap_version.m4


# To remove directories and other more complex lists of files we use the standard
# maintainer-clean-local target which is run after all the stuff in maintainer-clean
#
GENERATED_FILES = $(top_builddir)/.deps $(top_builddir)/.libs $(top_builddir)/* $(top_srcdir)/config

maintainer-clean-local:
	@echo 'maintainer-clean-local: Removing all generated files....'
	rm -rf $(GENERATED_FILES)
	rm -f $(top_srcdir)/m4/*.m4
	@echo 'maintainer-clean-local: Finished removing all generated files....'





mkdocdir:
	@if test ! -d docs ; then \
	  mkdir docs; \
	fi

docs: mkdocdir doxygen-doc



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
	  $(am__strip_dir) \
	  echo " $(INSTALL_DATA) '$$d$$p' '$(DESTDIR)/$(libdir)/$$f'"; \
	  $(INSTALL_DATA) "$$d$$p" "$(DESTDIR)/$(libdir)/$$f"; \
	done
	@echo "************************************************************"

@MACOSX_UNIVERSAL_FILE@:
	touch @MACOSX_UNIVERSAL_FILE@

