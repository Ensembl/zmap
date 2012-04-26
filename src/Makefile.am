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

SUBDIRS = $(ZMAP_SRC_DIRS) libpfetch libcurlobject libzmapfoocanvas include $(CANVAS_TESTING)

CLEANFILES = $(pkgconfig_DATA)

MOSTLYCLEANFILES = $(DX_CLEANFILES)




# toplevel files
# noinst_DATA = INSTALL


EXTRA_DIST = INSTALL \
	README.zmap \
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


#
# Libraries: code is compiled into a series of libXXX.a files, in some cases
# some of these are further combined into 'parent' libXXX.a files.
#

noinst_LTLIBRARIES = libZMapApp.la libZMapAppServices.la libZMap.la libZMapThr.la libZMapXML.la libZMapUtils.la \
                     libzmapfoocanvas.la libPFetch.la

libZMapXML_la_SOURCES = 
libZMapXML_la_LIBADD  = zmapXML/libZMapXML.la

libZMapUtils_la_SOURCES = 
libZMapUtils_la_LIBADD  = zmapUtils/libZMapUtils.la

libZMapAppServices_la_SOURCES = 
libZMapAppServices_la_LIBADD  = zmapAppServices/libZMapAppServices.la

libZMapApp_la_SOURCES =
libZMapApp_la_LIBADD  = zmapApp/libZMapApp.la

# try moving server stuff to here....
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

libZMapThr_la_SOURCES =
libZMapThr_la_LIBADD  =       \
zmapThreads/libZMapThreads.la \
$(NULL)

libPFetch_la_SOURCES = 
libPFetch_la_LIBADD  = libpfetch/libpfetch.la libcurlobject/libcurlobject.la

libzmapfoocanvas_la_SOURCES = 
libzmapfoocanvas_la_LIBADD  = libzmapfoocanvas/libzmapfoocanvas.la


# Programs we're going to compile.

bin_PROGRAMS = zmap gffparser xml_writer xremote_gui


# Try Jeremy's new build stuff....
#
#zmap_LDADD = $(X_PRE_LIBS) $(X_LIBS) $(X_EXTRA_LIBS)         \
#	-L$(top_builddir)/.libs                              \
#	-lZMapApp -lZMap -lZMapThr -lZMap -lZMapXML -lPFetch \
#	$(ACECONN_LIBS) $(GTK_LIBS) $(FOOCANVAS_LIBS)        \
#	$(LIBCURL) -lexpat -lg2 -lgd                         \
#	@MACOSX_UNIVERSAL_FLAGS@ $(NULL)


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
	$(top_builddir)/.libs/libZMapThr.a                   \
	$(top_builddir)/.libs/libZMapXML.a                   \
	$(top_builddir)/.libs/libZMapAppServices.a           \
	$(top_builddir)/.libs/libZMapUtils.a                 \
	$(top_builddir)/.libs/libzmapfoocanvas.a             \
	$(top_builddir)/.libs/libPFetch.a                    \
        $(X_PRE_LIBS) $(X_LIBS) $(X_EXTRA_LIBS)              \
	$(ACECONN_LIBS) $(GTK_LIBS)                          \
	$(LIBCURL) -lexpat -lg2 -lgd                         \
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
xremote_gui_SOURCES      = $(top_srcdir)/zmapControl/remote/xremote_gui_test.c
xremote_gui_LDFLAGS      = 
xremote_gui_LDADD        = $(zmap_LDADD)
xremote_gui_DEPENDENCIES = $(noinst_LTLIBRARIES)
xremote_gui_CPPFLAGS     = $(AM_CPPFLAGS) -I$(top_srcdir)/zmapApp



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




################################################################################
################################################################################
# Extra targets - specified by us !!
################################################################################
################################################################################


#------------ start of compiledate file targets ------------------------

# This is the code that gets built by the build-compiledate-file target. We create
# this code dynamically because we don't want it in our code repository because
# it will change with every build.
#
ZMAP_COMPILEDATE_FILE = zmapCompileDate.c
ZMAP_COMPILEDATE_PATH = zmapUtils/$(ZMAP_COMPILEDATE_FILE)
ZMAP_COMPILEDATE_FILE_CODE =                            \
                                                    \
char *zmapCompileString(void)                       \
{                                                   \
  return ZMAP_COMPILE_STRING() ;                    \
}                        


# Here's the target that creates the file containing the ZMAP_COMPILEDATE_FILE_CODE.
# By making build-compiledate-file depend on the empty forcemake target it gets
# remade every time.
#
forcemake:

build-compiledate-file: forcemake
	if test -f $(top_srcdir)/runconfig ; then \
	echo "creating compiledate file....." ; \
	echo "/* Do not Edit - generated by makefile target $@ */" > $(top_srcdir)/$(ZMAP_COMPILEDATE_PATH) ; \
	echo "#include <zmapUtils_P.h>" >> $(top_srcdir)/$(ZMAP_COMPILEDATE_PATH) ; \
	echo "$(ZMAP_COMPILEDATE_FILE_CODE)" >> $(top_srcdir)/$(ZMAP_COMPILEDATE_PATH) ; \
	echo "created compiledate file....." ; fi


# This is the way we ensure that build-compiledate-file is included in the automake
# generated 'all' and 'dist' targets.
#
# This isn't pretty but I have not found a better way to _prepend_ our target to
# the 'all' and 'dist' targets. NOTE that you _cannot_ use the nnn-local targets,
# e.g. all-local, to do this because they are run _after_ all the other stuff.
#
# I have simply copied these targets from the Makefile generated by automake for
# our build and added in our build-compiledate-file target. Because these come after
# automake's versions of the targets in the Makefile, make overwrites automake's
# versions with our versions...and gives a warning that it is doing this. Not
# pretty but it seems to work. Surely there is a more elegant way.....
#
all: config.h build-compiledate-file
	$(MAKE) $(AM_MAKEFLAGS) all-recursive

# This doesn't work because the original "all" target also has to have a double colon...sigh
#all:: build-compiledate-file

dist dist-all: build-compiledate-file distdir
	tardir=$(distdir) && $(am__tar) | GZIP=$(GZIP_ENV) gzip -c >$(distdir).tar.gz
	$(am__remove_distdir)


#------------ end of compiledate file targets --------------------------


# Our additions to some standard targets. (the XXX-local targets are
# automakes way of allowing us to add stuff to standard targets such
# as "all" etc.
#
mkdocdir:
	@if test ! -d docs ; then \
	  mkdir docs; \
	fi

docs: mkdocdir doxygen-doc

maintainer-clean-local:
	rm -rf cleanDir

distclean-local:
	rm -f $(top_srcdir)/m4/*.m4

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
