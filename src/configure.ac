#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
################################################################################
################################################################################
# Initialise
################################################################################
################################################################################

AC_PREREQ(2.57)
AC_INIT([ZMap],[0.1],[zmap@sanger.ac.uk])

# check we can find the source
AC_CONFIG_SRCDIR([zmapApp/zmapAppmain_c.c])
AC_CONFIG_HEADER(config.h)

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
   # this is pretty evil NEED to find a better method
   ALPHA_POPT=-I/sw/include
   AC_SUBST(ALPHA_POPT)
else
   # this is pretty evil NEED to find a better method
   ALPHA_POPT=-I/nfs/disk100/acedb/prefix/ALPHA/include
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

AC_ARG_ENABLE(shtml-doc,
        AC_HELP_STRING([--enable-shtml-doc], [enable shtml documentation info compilation (no)]),
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


#AC_PROG_LIBTOOL
#AC_DISABLE_SHARED
AM_INIT_AUTOMAKE

AC_LANG(C)

# for some reason AC_PATH_XTRA doesn't set up -lX11
AC_PATH_XTRA
X_LIBS="$X_LIBS -lX11"

AC_ARG_ENABLE(debug,
        AC_HELP_STRING([--enable-debug], [enable debug info compilation (no)]),
        deb=$enableval, deb=no)

if test "x$deb" = "xyes"; then
   CFLAGS="$CFLAGS -g";
else
   CFLAGS="$CFLAGS -O2";
fi


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
AC_PROG_RANLIB
AC_PATH_PROGS(PERL, perl perl5)
AC_PROG_LN_S
################################################################################
################################################################################
# Checks for libraries.
################################################################################
################################################################################

# gtk first

GTK2_MODULES="pango >= 0.1 gtk+-2.0 glib-2.0 gthread-2.0"
PKG_CHECK_MODULES([GTK], [${GTK2_MODULES}])

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

AC_CHECK_LIB(AceConn, [main], [],AC_MSG_ERROR(where is AceConn), -lmd5)



#AC_CHECK_LIB(popt, [poptFreeContext], [],AC_MSG_ERROR(where is popt))

# and substitute values....
# Should be able to use @VARIABLE@ in Makefile.am
AC_SUBST(FOOCANVAS_CFLAGS)
AC_SUBST(FOOCANVAS_LIBS)
AC_SUBST(ACECONN_CFLAGS)
AC_SUBST(ACECONN_LIBS)
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)
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
AC_CHECK_HEADER([AceConn.h], [], [], [])
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

LDFLAGS="$FOOCANVAS_LIBS $ACECONN_LIBS"
AC_CHECK_LIB(foocanvas, foo_canvas_zmap,:, AC_MSG_ERROR(no patched libfoocanvas!))
# HAVE_LIBFOOCANVAS is now defined

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
zmapApp/Makefile
zmapConfig/Makefile
zmapControl/Makefile
zmapControl/remote/Makefile
zmapDocs/Makefile
zmapDraw/Makefile
zmapFeature/Makefile
zmapGFF/Makefile
zmapMake/Makefile
zmapManager/Makefile
zmapServer/Makefile
zmapServer/acedb/Makefile
zmapServer/das/Makefile
zmapServer/file/Makefile
zmapThreads/Makefile
zmapUtils/Makefile
zmapView/Makefile
zmapWindow/Makefile
zmapXML/Makefile
include/Makefile
lib/Makefile
zmap.pc
ZMap:dotZmap.in
$srcdir/$DX_CONFIG:Doxyfile.in
])

AC_OUTPUT
