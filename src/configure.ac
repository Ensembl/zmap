#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
################################################################################
################################################################################
# Initialise
################################################################################
################################################################################

AC_PREREQ(2.59)
AC_INIT([ZMap],[0.1],[zmap@sanger.ac.uk])

# check we can find the source
AC_CONFIG_SRCDIR([zmapApp/zmapAppmain_c.c])
AC_CONFIG_HEADER(config.h)

echo "************************************************************"
echo "* Welcome to ./configure for $PACKAGE_STRING"
echo "* Cmd: $0 $@"
echo "* Environment:"
echo "*   LDFLAGS          : $LDFLAGS"
echo "*   CFLAGS           : $CFLAGS"
echo "*   CC               : $CC"
echo "*   PATH             : $PATH"
echo "*   PACKAGE_CFG      : $PKG_CONFIG"
echo "*   PACKAGE_CFG_PATH : $PKG_CONFIG_PATH"
echo "*   LD_LIBRARY_PATH  : $LD_LIBRARY_PATH"
echo "* continuing..."
echo "*"


# what are we building on?
AC_CANONICAL_BUILD
case $build in
     *-*-linux*)  
     zmap_os_type="Linux";;
     *-*-darwin*) 
     zmap_os_type="darwin";;
     *) 
     zmap_os_type="Unknown";;
esac
# replaces -DLINUX
if test "$zmap_os_type" = "Linux" ; then
   AC_MSG_RESULT([Got OS type of LINUX])
   AC_DEFINE(LINUX, [], [Operating System is LINUX.  See zmapThreads/zmapThreads_P.h])
elif test "$zmap_os_type" = "darwin" ; then
   AC_MSG_RESULT([Got OS type of Darwin])
   AC_DEFINE(DARWIN, [], [Operating System is DARWIN.  See zmapThreads/zmapThreads_P.h])

   # this is pretty evil NEED to find a better method_
#   ALPHA_POPT=-I/sw/include
#   AC_SUBST(ALPHA_POPT)
else
   # this is pretty evil NEED to find a better method
   ALPHA_POPT=-I/nfs/team71/acedb/prefix/ALPHA/include
   AC_SUBST(ALPHA_POPT)
fi

################################################################################
################################################################################
# Doxygen Features

DX_HTML_FEATURE(ON)
DX_CHM_FEATURE(OFF)
DX_CHI_FEATURE(OFF)
DX_MAN_FEATURE(OFF)
DX_RTF_FEATURE(OFF)
DX_XML_FEATURE(OFF)
DX_PDF_FEATURE(ON)
DX_PS_FEATURE(OFF)
DX_INIT_DOXYGEN([AC_PACKAGE_NAME], Doxyfile, docs/[AC_PACKAGE_NAME])

ZMAP_FALSE=no
ZMAP_TRUE=yes

AC_ARG_WITH([macosx-sdk-path],
        AS_HELP_STRING([--with-maxosx-sdk-path=path], [enable building of mac osx universal binary using 'path' to SDK]),
        [], [with_macosx_sdk_path=$ZMAP_FALSE])

MACOSX_UNIVERSAL_FILE=".deps/zmap.c"
MACOSX_UNIVERSAL_FLAGS=""
AS_IF([test "x$with_macosx_sdk_path" != "x$ZMAP_FALSE"],
	    AS_IF([ test -d $with_macosx_sdk_path ],
	    [[
	    # SDK path
	    export SDK=$with_macosx_sdk_path
	    # This stops link errors to -lcrt1.so
	    export MACOSX_DEPLOYMENT_VERSION=10.4
	    CFLAGS="$CFLAGS -arch i386 -arch ppc -isysroot $SDK -mmacosx-version-min=$MACOSX_DEPLOYMENT_VERSION"
	    # we need to make another target. N.B. _single_ quoted
	    MACOSX_UNIVERSAL_FILE='$(top_srcdir)/zmapApp/.zmap.c'
	    MACOSX_UNIVERSAL_FLAGS='-Wl,-undefined,dynamic_lookup -Wl,-all_load'
	    MACOSX_UNIVERSAL_FLAGS=''
	    ###
	    ]], [with_macosx_sdk_path=$ZMAP_FALSE]))

AC_SUBST(MACOSX_UNIVERSAL_FILE)
AC_SUBST(MACOSX_UNIVERSAL_FLAGS)
# autoconf can forget to subst abs_top_srcdir
AC_SUBST(abs_top_srcdir)

AC_ARG_ENABLE(shtml-doc,
        AS_HELP_STRING(--enable-shtml-doc,enable shtml documentation info compilation (no)),
        shtml=$enableval, shtml=no)

ZMAP_DOXY_HTML_FILE_EXT=".html"
ZMAP_DOXY_HTML_HEADER=""
ZMAP_DOXY_HTML_FOOTER=""

if test "x$shtml" = "xyes"; then
  ZMAP_DOXY_HTML_FILE_EXT=".shtml";
  ZMAP_DOXY_HTML_HEADER="doxyhtml.hdr";
  ZMAP_DOXY_HTML_FOOTER="doxyhtml.ftr";
fi
AC_SUBST(ZMAP_DOXY_HTML_FILE_EXT)
AC_SUBST(ZMAP_DOXY_HTML_HEADER)
AC_SUBST(ZMAP_DOXY_HTML_FOOTER)


AC_PROG_LIBTOOL
AM_INIT_AUTOMAKE

AC_LANG(C)

# for some reason AC_PATH_XTRA doesn't set up -lX11
AC_PATH_XTRA
X_LIBS="$X_LIBS -lX11"

PKG_CHECK_MODULES([CHECK], [check >= 0.9.4])

# Leopard and tiger have different versions of X11 installed in different places.
# Leopard has a softlink for the tiger directory and as the tiger version is older we go 
# with that one, of which there's a copy in the 10.4u sdk.
if test "x$with_macosx_sdk_path" != "x$ZMAP_FALSE" ; then
   X_LIBS="-L$with_macosx_sdk_path/usr/X11R6/lib -lX11"
fi

AC_ARG_ENABLE(debug,
        AS_HELP_STRING(--enable-debug,enable debug info compilation (no)),
        deb=$enableval, deb=no)

AC_ARG_ENABLE(profile,_
        AS_HELP_STRING(--enable-profile,enable profiling debug info compilation (no)),
        prof=$enableval, prof=no)

if test "x$prof" = "xyes"; then
   CFLAGS="$CFLAGS -pg -g";
elif test "x$deb" = "xyes"; then
   CFLAGS="$CFLAGS -g";
else
   CFLAGS="$CFLAGS -O2";
fi

AC_SUBST(PKG_CONFIG)
AC_SUBST(CFLAGS)
AC_SUBST(CPPFLAGS)
AC_SUBST(LDFLAGS)
VERSION="0.1"
AC_SUBST(VERSION)
################################################################################
################################################################################
# Checks for programs.
################################################################################
################################################################################

AC_PROG_CXX
AC_PROG_CC
#AC_PROG_RANLIB
AC_PATH_PROGS(PERL, perl perl5)
AC_PROG_LN_S

AC_ARG_VAR(GLIB_GENMARSHAL, [The glib-genmarshal executable.])
AC_CHECK_PROG(GLIB_GENMARSHAL, glib-genmarshal, glib-genmarshal)


################################################################################
################################################################################
# Checks for libraries.
################################################################################
################################################################################

# gtk first

GTK2_MODULES="pango >= 0.1 gtk+-2.0 >= 2.4.0 glib-2.0 gthread-2.0"
PKG_CHECK_MODULES([GTK], [${GTK2_MODULES}])

CANVAS_TESTING=""
GTK2_MODULES="gtk+-2.0 >= 2.10.0"
PKG_CHECK_MODULES([LATESTGTK], [${GTK2_MODULES}], [
  CANVAS_TESTING="canvas_choice"
  AC_MSG_RESULT([Found GTK 2.10])
], [
  CANVAS_TESTING=""
  AC_MSG_RESULT([GTK 2.10 not found.])
])

DEMO_CANVAS_MODULES="crcanvas >= 0.12 goocanvas >= 0.7 geocanvas >= 1.0.7"
PKG_CHECK_MODULES([DEMO_CANVAS], [${DEMO_CANVAS_MODULES}], [
  AC_MSG_RESULT([Found Canvases])
], [
  if test "x$CANVAS_TESTING" != "x"; then
    CANVAS_TESTING=""
    AC_MSG_RESULT([Are you sure?])
  else
    AC_MSG_RESULT([Canvases not found, but neither was GTK 2.10])
  fi                               
])

AC_SUBST([CANVAS_TESTING])


dnl This has to go here for our nfs mounted mac....    
if [ test "$zmap_os_type" = "darwin" ] && [ test -d /Users/zmap/nfs_mount ]; then
  PKG_CONFIG="$PKG_CONFIG --define-variable MOUNT_POINT=/Users/zmap/nfs_mount"
# man pkg-config says  use --define-variable=VAR=VALUE, but 
# pkg-config --define-variable=VAR=VALUE --atleast-pkgconfig-version 0.9.0 
# fails whereas
# pkg-config --define-variable VAR=VALUE --atleast-pkgconfig-version 0.9.0
# does not
fi

# now foocanvas
# really we want to allow foocanvas and foocanvas-patched(our version)
# to live side by side but for now lets leave it like this
PKG_CHECK_MODULES([FOOCANVAS], [libfoocanvas = 0.1],,AC_MSG_ERROR(foocanvas is required))

# needs curl, we'll be looking using curl-config
# LIBCURL_CHECK_CONFIG ([DEFAULT-ACTION], [MINIMUM-VERSION],
#                       [ACTION-IF-YES], [ACTION-IF-NO])
LIBCURL_CHECK_CONFIG([],7.11.0,[AC_MSG_RESULT(LIBCURL FLAGS ${LIBCURL})],[AC_MSG_ERROR(Sorry we need at least version 7.11.0)])

PKG_CHECK_MODULES([ACECONN], [AceConn-1.1],,AC_MSG_ERROR(AceConn is required))

CPPFLAGS="$CPPFLAGS $ACECONN_CFLAGS"
LDFLAGS="$LDFLAGS $ACECONN_LIBS"

AC_CHECK_LIB(AceConn, [main],
  [ have_ace_conn='true' ], 
  [ have_ace_conn='false';
    AC_MSG_ERROR(AceConn package is required) ], 
  [-lmd5])

if test "x$with_macosx_sdk_path" = "x$ZMAP_FALSE" ; then
    AC_CHECK_LIB(g2, [g2_attach],  
    [ have_g2='true';
      AC_CHECK_LIB(g2, [g2_open_gd], 
        [ have_g2_gd='true' ], 
	[ have_g2_gd='false';
          AC_MSG_ERROR(g2 must have GD compiled in) ], [-lgd] )], 
	[ have_g2=have_g2_gd='false';
	    AC_MSG_ERROR(g2 package is required) ])
fi

#AC_CHECK_LIB(popt, [poptFreeContext], [],AC_MSG_ERROR(where is popt))

# and substitute values....
# Should be able to use @VARIABLE@ in Makefile.am
AC_SUBST(FOOCANVAS_CFLAGS)
AC_SUBST(FOOCANVAS_LIBS)
AC_SUBST(ACECONN_CFLAGS)
AC_SUBST(ACECONN_LIBS)
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)
AC_SUBST(DEMO_CANVAS_CFLAGS)
AC_SUBST(DEMO_CANVAS_LIBS)

# keep track for rpath
#RPATH=""
#full_paths="${FOOCANVAS_LIBS} ${ACECONN_LIBS} ${GTK_LIBS}"
#if $full_paths ; then
#   save_ifs=$IFS; IFS=':'
#   for l in $full_paths
#    do
#      case $l in
#           -L*) RPATH="${RPATH} $l";;
#      esac      
#   done
#   IFS=$save_ifs
#fi
#echo ${RPATH}
#zmapMake/platform.alpha_5:LDFLAGS = -rpath '/usr/local/gtk2/lib:$(POPT_LIB)'
#zmapMake/platform.linux_4:LDFLAGS = -Xlinker -rpath -Xlinker $(FOO_CANVAS_PATH)/lib
# -rpath :separated_list
# -Wl,-rpath 

################################################################################
################################################################################
# Checks for header files.
################################################################################
################################################################################

AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h locale.h stdlib.h string.h strings.h sys/param.h unistd.h])
AC_CHECK_HEADER([AceConn.h g2.h g2_gd.h], [], [], [])
AC_CHECK_HEADER([popt.h], [], [], [])
################################################################################
################################################################################
# Checks for typedefs, structures, and compiler characteristics.
################################################################################
################################################################################
AC_C_CONST
AC_TYPE_SIZE_T

################################################################################
################################################################################
# Checks for library functions.
################################################################################
################################################################################
AC_FUNC_STAT
AC_CHECK_FUNCS([gethostname memmove memset mkdir strcasecmp strstr strtol])

LDFLAGS="$LDFLAGS $FOOCANVAS_LIBS $ACECONN_LIBS"
if test "x$with_macosx_sdk_path" == "x$ZMAP_FALSE" ; then
  AC_CHECK_LIB(foocanvas, foo_canvas_zmap,:, AC_MSG_ERROR(no patched libfoocanvas!))
# HAVE_LIBFOOCANVAS is now defined
fi

#--------------------------------------------------------------------
#	Find out if backtrace/backtrace_symbols_fd are available.
#--------------------------------------------------------------------
AC_CHECK_HEADER([execinfo.h], [
			      AC_CHECK_FUNCS([backtrace backtrace_symbols_fd], [AC_DEFINE([HAVE_GNUC_BACKTRACE], [1], [Have gnu glibc and backtrace()])], [])
			      ], 
			      [AC_DEFINE([NO_BACKTRACE], [], [There's no backtrace from gnu glibc on this machine])], 
			      [#ifndef NO_EXTRA_HEADERS
			      #endif /* EXTRA_HEADERS */
			      ])

#--------------------------------------------------------------------
#	Find out if sigaction is available.
#--------------------------------------------------------------------
AC_CHECK_FUNC([sigaction],[sigaction=1],[sigaction=0])
if test $sigaction = 0; then
  AC_DEFINE([NO_SIGACTION], [], [There's no sigaction() on this machine])
fi

################################################################################
################################################################################
# Stuff for ~/.ZMap directory
################################################################################
################################################################################
# make this if "x$ac_with_blah" = "xyes" then blah else no_blah
ZMAPSCHEME=acedb
ZMAPUSERNAMEPASSWORD="any:any@"
ZMAPHOSTPORT="$ac_hostname:23100"
ZMAPSTYLESFILE="ZMapTypes.b0250"
AC_SUBST(ZMAPSCHEME)
AC_SUBST(ZMAPUSERNAMEPASSWORD)
AC_SUBST(ZMAPHOSTPORT)
AC_SUBST(ZMAPSTYLESFILE)

# THE zmap* source directories
ZMAP_SRC_DIRS="zmapApp \
zmapConfig \
zmapControl \
zmapDAS \
zmapDocs \
zmapDraw \
zmapFeature \
zmapGFF \
zmapMake \
zmapManager \
zmapServer \
zmapThreads \
zmapUtils \
zmapView \
zmapWindow \
zmapXML"

AC_SUBST(ZMAP_SRC_DIRS)
ZMAP_SRC_DIRS_DOXYGEN=""
for zm_srcdir in $ZMAP_SRC_DIRS
do
   ZMAP_SRC_DIRS_DOXYGEN="$ZMAP_SRC_DIRS_DOXYGEN $srcdir/$zm_srcdir"
done
AC_SUBST(ZMAP_SRC_DIRS_DOXYGEN)


AC_CONFIG_FILES([Makefile
canvas_choice/Makefile
zmapApp/Makefile
zmapConfig/Makefile
zmapControl/Makefile
zmapControl/remote/Makefile
zmapDocs/Makefile
zmapDAS/Makefile
zmapDraw/Makefile
zmapFeature/Makefile
zmapFeature/tests/Makefile
zmapGFF/Makefile
zmapMake/Makefile
zmapManager/Makefile
zmapServer/Makefile
zmapServer/acedb/Makefile
zmapServer/das/Makefile
zmapServer/file/Makefile
zmapServer/pipe/Makefile
zmapThreads/Makefile
zmapUtils/Makefile
zmapUtils/tests/Makefile
zmapView/Makefile
zmapWindow/Makefile
zmapXML/Makefile
include/Makefile
libpfetch/Makefile
libpfetch/tests/Makefile
libcurlobject/Makefile
zmap.pc
dotZMap:dotZmap.in
$srcdir/$DX_CONFIG:Doxyfile.in
])

AC_OUTPUT
