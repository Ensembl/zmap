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



# ZMap dirs, see configure.ac for how these vars are set.

if USE_GBTOOLS
OPTIONAL_SUBDIRS = gbtools
endif

if USE_ENSEMBL_SUBDIR
OPTIONAL_SUBDIRS = ensc-core/src
endif

SUBDIRS = $(OPTIONAL_SUBDIRS) libzmapfoocanvas

if USE_ACECONN
SUBDIRS += AceConn
endif

if USE_ENSEMBL
if USE_ENSEMBL_SUBDIR
SUBDIRS += ensc-core/src 
endif
endif

if USE_HTSLIB
SUBDIRS += htslib
endif

if USE_ZEROMQ
SUBDIRS += zeromq
endif

SUBDIRS += include $(ZMAP_SRC_DIRS) $(CANVAS_TESTING)


CLEANFILES = $(pkgconfig_DATA)

MOSTLYCLEANFILES = $(DX_CLEANFILES)




# toplevel files
# noinst_DATA = INSTALL


# these will be installed alongside the programs
USER_DIR=$(srcdir)/doc/user
dist_doc_DATA = $(USER_DIR)/Splice_highlight_off.png \
	$(USER_DIR)/config_file.shtml \
	$(USER_DIR)/keyboard_mouse.shtml \
	$(USER_DIR)/filter_window.shtml \
	$(USER_DIR)/styles.shtml \
	$(USER_DIR)/zmap_quick_start.shtml \
	$(USER_DIR)/Splice_highlight_on.png \
	$(USER_DIR)/configuration.shtml \
	$(USER_DIR)/manual/ZMap_User_Manual.pdf \
	$(USER_DIR)/this_links.ini \
	$(USER_DIR)/align1.png \
	$(USER_DIR)/index.shtml \
	$(USER_DIR)/release_notes.shtml \
	$(USER_DIR)/user_interface.shtml

# these are included in the distribution, but are not installed
EXTRA_DIST = INSTALL \
	INSTALL.linux \
	INSTALL.mac \
	INSTALL.windows \
	README \
	$(srcdir)/doc/* \
	$(srcdir)/examples/* \
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

noinst_LTLIBRARIES = libZMapApp.la libZMap.la \
                     libZMapAppServices.la libZMapRemoteControl.la \
                     libZMapDataSource.la libZMapThreads.la libZMapServer.la libZMapServers.la \
                     libZMapThreadsLib.la libZMapXML.la libZMapUtils.la \
                     libzmapfoocanvas.la 


libZMapApp_la_SOURCES =
libZMapApp_la_LIBADD  = zmapApp/libZMapApp.la

libZMap_la_SOURCES =
libZMap_la_LIBADD  = \
zmapControl/libZMapControl.la \
zmapConfig/libZMapConfig.la   \
zmapDraw/libZMapDraw.la       \
zmapFeature/libZMapFeature.la \
zmapGFF/libZMapGFF.la         \
zmapView/libZMapView.la       \
zmapWindow/libZMapWindow.la   \
$(NULL)

libZMapAppServices_la_SOURCES = 
libZMapAppServices_la_LIBADD  = zmapAppServices/libZMapAppServices.la

libZMapRemoteControl_la_SOURCES = 
libZMapRemoteControl_la_LIBADD  = zmapRemoteControl/libZMapRemoteControl.la

libZMapDataSource_la_SOURCES = 
libZMapDataSource_la_LIBADD  = zmapDataSource/libZMapDataSource.la

libZMapThreads_la_SOURCES =
libZMapThreads_la_LIBADD  = zmapThreads/libZMapThreads.la

libZMapServer_la_SOURCES = 
libZMapServer_la_LIBADD  = zmapServer/libZMapServer.la

libZMapServers_la_SOURCES = 
libZMapServers_la_LIBADD  = zmapServer/libZMapServers.la

libZMapThreadsLib_la_SOURCES =
libZMapThreadsLib_la_LIBADD  = zmapThreadsLib/libZMapThreadsLib.la

libZMapXML_la_SOURCES = 
libZMapXML_la_LIBADD  = zmapXML/libZMapXML.la

libZMapUtils_la_SOURCES = 
libZMapUtils_la_LIBADD  = zmapUtils/libZMapUtils.la

libzmapfoocanvas_la_SOURCES =
libzmapfoocanvas_la_LIBADD  = libzmapfoocanvas/libzmapfoocanvas.la


# Programs we're going to compile.

#bin_PROGRAMS = zmap gffparser xml_writer xremote_gui remotecontrol
# bin_PROGRAMS = zmap xml_writer remotecontrol

bin_PROGRAMS = zmap

if USE_ZEROMQ
bin_PROGRAMS += remotecontrol
endif

# I am perturbed by the fact that the x libs are before the gtk libs....
#

#zmap_SOURCES      = $(top_srcdir)/zmapApp/zmapAppmain_c.c
zmap_SOURCES      = $(top_srcdir)/zmapApp/zmapAppmain_c.cpp


zmap_LDADD =                                                 \
	$(top_builddir)/.libs/libZMapApp.a                   \
	$(top_builddir)/.libs/libZMap.a                      \
	$(top_builddir)/.libs/libZMapAppServices.a           \
	$(top_builddir)/.libs/libZMapRemoteControl.a         \
	$(top_builddir)/.libs/libZMapDataSource.a            \
	$(top_builddir)/.libs/libZMapThreads.a               \
	$(top_builddir)/.libs/libZMapServer.a                \
	$(top_builddir)/.libs/libZMapServers.a               \
	$(top_builddir)/.libs/libZMapThreadsLib.a            \
	$(top_builddir)/.libs/libZMapXML.a                   \
	$(top_builddir)/.libs/libZMapUtils.a                 \
	$(top_builddir)/.libs/libzmapfoocanvas.a 


if USE_ACECONN
zmap_LDADD += $(top_builddir)/AceConn/src/.libs/libAceConn.a 
endif

if USE_ENSEMBL
if USE_ENSEMBL_SUBDIR
zmap_LDADD += $(top_builddir)/ensc-core/src/.libs/libensc.a
endif
endif

if USE_HTSLIB
zmap_LDADD += $(top_builddir)/htslib/libhts.a
endif

# If we have gbtools in a subdirectory, add the .a file, otherwise
# look for a locally installed version
if USE_GBTOOLS
zmap_LDADD += $(top_builddir)/gbtools/.libs/libgbtools.a            
else
zmap_LDADD += -lgbtools
endif

# The zeromq library has a different suffix on cygwin
if USE_ZEROMQ
zmap_LDADD += $(top_builddir)/zeromq/src/.libs/libzmq.a            
endif


# I've had to add in the X11 lib otherwise we can't compile on Ubuntu
# 64-bit machines (-lz is included because htslib needs it).
zmap_LDADD +=                                                \
        $(X_PRE_LIBS) $(X_LIBS) $(X_EXTRA_LIBS)              \
	$(GTK_LIBS)			                     \
	$(LIBCURL) -lexpat -lX11 -lz                         

if USE_ENSEMBL
zmap_LDADD += $(MYSQL_LDFLAGS)                                     
endif

zmap_LDADD +=                                                \
	@MACOSX_UNIVERSAL_FLAGS@ $(NULL)

zmap_DEPENDENCIES = $(noinst_LTLIBRARIES) @MACOSX_UNIVERSAL_FILE@
zmap_CFLAGS       = $(AM_CPPFLAGS) -I$(top_srcdir)/zmapApp

if USE_ENSEMBL
zmap_CFLAGS += $(MYSQL_CFLAGS)
endif

# zeromq is C++ so we need to link with g++, not gcc.
#zmap_LINK         = $(CC) $(AM_CFLAGS) $(CFLAGS) $(AM_LDFLAGS) $(LDFLAGS) -o $@
zmap_LINK         = $(CXX) -I$(top_srcdir)/zmapApp $(AM_CFLAGS) $(CFLAGS) $(AM_LDFLAGS) $(LDFLAGS) -o $@




# gffparser - program to parse gff...
#gffparser_SOURCES      = $(top_srcdir)/zmapGFF/gffparser.c
#gffparser_LDFLAGS      =
#gffparser_LDADD        = $(zmap_LDADD)
#gffparser_DEPENDENCIES = $(noinst_LTLIBRARIES)
#gffparser_CPPFLAGS     = $(AM_CPPFLAGS)

# test program to write xml...
xml_writer_SOURCES      = $(top_srcdir)/zmapXML/example_xml_writer.cpp
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
#remotecontrol_SOURCES      = $(top_srcdir)/zmapRemoteControl/remotetest/remotecontroltest.c
remotecontrol_SOURCES      = $(top_srcdir)/zmapRemoteControl/remotetest/remotecontroltest.cpp
remotecontrol_LDFLAGS      = 
remotecontrol_LDADD        = $(zmap_LDADD)
remotecontrol_DEPENDENCIES = $(noinst_LTLIBRARIES)
remotecontrol_CPPFLAGS     = $(AM_CPPFLAGS) -I$(top_srcdir)/zmapApp
remotecontrol_LINK         = $(CXX)  $(AM_CFLAGS) $(CFLAGS) $(AM_LDFLAGS) $(LDFLAGS) -o $@

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
check_zmap_SOURCES      = $(top_srcdir)/tests/check_zmapmain.cpp
check_zmap_DEPENDENCIES = $(check_LTLIBRARIES)
check_zmap_LINK         = $(zmap_LINK)
check_zmap_LDADD        = @CHECK_LIBS@ -lcheckZMap $(zmap_LDADD)
check_zmap_CFLAGS       = @CHECK_CFLAGS@ $(INCLUDE_COMMON) \
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


# Standard way to add _files_ to the list deleted by the maintainer-clean target, 
# cannot be used for directories.
MAINTAINERCLEANFILES = $(top_srcdir)/Makefile.in $(top_srcdir)/aclocal.m4 \
$(top_srcdir)/config.h.in $(top_srcdir)/configure $(top_srcdir)/zmap_version.m4


# To remove directories and other more complex lists of files we use the standard
# maintainer-clean-local target which is run after all the stuff in maintainer-clean
#
GENERATED_FILES = $(top_builddir)/.deps $(top_builddir)/.libs $(top_builddir)/* $(top_srcdir)/config \
$(top_srcdir)/autom4te.cache

EXTERNAL_LIBS = $(top_srcdir)/AceConn $(top_srcdir)/ensc-core $(top_srcdir)/gbtools \
$(top_srcdir)/htslib $(top_srcdir)/zeromq
EXTERNAL_PLACEHOLDERS = $(top_srcdir)/AceConn $(top_srcdir)/ensc-core \
$(top_srcdir)/gbtools $(top_srcdir)/htslib $(top_srcdir)/zeromq

maintainer-clean-local:
	@echo 'maintainer-clean-local: Removing all generated files....'
	rm -rf $(GENERATED_FILES)
	rm -f $(top_srcdir)/m4/*.m4
	@echo 'maintainer-clean-local: Finished removing all generated files....'
	@echo 'maintainer-clean-local: Removing all external library files....'
	rm -rf $(EXTERNAL_LIBS)
	git checkout $(EXTERNAL_PLACEHOLDERS)
	@echo 'maintainer-clean-local: Finished removing all external library files....'


# not needed now ?
ZMAP_BUILD_FILES = $(top_builddir)/zmapApp/*.o $(top_builddir)/zmapAppServices/*.o $(top_builddir)/zmapConfig/*.o \
$(top_builddir)/zmapControl/*.o $(top_builddir)/zmapDataSource/*.o $(top_builddir)/zmapDraw/*.o \
$(top_builddir)/zmapFeature/*.o \
$(top_builddir)/zmapGFF/*.o $(top_builddir)/zmapManager/*.o $(top_builddir)/zmapRemoteControl/*.o \
$(top_builddir)/zmapServer/*.o $(top_builddir)/zmapThreadsLib/*.o $(top_builddir)/zmapThreads/*.o $(top_builddir)/zmapUtils/*.o \
$(top_builddir)/zmapView/*.o $(top_builddir)/zmapWindow/*.o $(top_builddir)/zmapXML/*.o

zmap-clean:
	@echo 'clean-zmap: starting cleaning up all zmap build files....'
	@for i in `ls -d ./zmap*` ; \
	do { if [ -d $i ] ; then ( cd $i ; echo make clean $i ) fi ; } \
	done
	@echo 'clean-zmap: finished cleaning up all zmap build files.'



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
	  echo " $(INSTALL_DATA) '$$d$$p' '$(DESTDIR)$(libdir)/$$f'"; \
	  $(INSTALL_DATA) "$$d$$p" "$(DESTDIR)$(libdir)/$$f"; \
	done
	@echo "************************************************************"

@MACOSX_UNIVERSAL_FILE@:
	touch @MACOSX_UNIVERSAL_FILE@

#
# location on tviewsrv is '/usr/local/qt5/5.5/gcc/bin/qmake' and the command to use is of the form:
#/usr/local/Qt5.5.0/5.5/gcc_64/bin/qmake /nfs/users/nfs_s/sm23/Work/TestCode/QT/QT01/QT01.pro -r -spec linux-g++ ;\
#
# OK so this just has my test program in place temporarily to check that things work; eventually this
# will become something of the form: $(QMAKE_LOCAL) $(ZMAP_SRC_DIRS)
#
include qmake.location
test-qt:
	@echo "make test-qt"
	mkdir $(top_builddir)/test-qt ;\
	cd $(top_builddir)/test-qt ;\
        $(QMAKE_LOCAL) /nfs/users/nfs_s/sm23/Work/TestCode/QT/QT01/QT01.pro $(QMAKE_ARGS) ;\
	make ;\
	cd $(top_builddir)

test-qt-clean:
	@echo "make test-qt-clean"
	rm -rf $(top_builddir)/test-qt

