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

# what are we building on?
AC_CANONICAL_BUILD
case $build in
     *-*-linux*) 
     zmap_os_type="Linux";;
     *) 
     zmap_os_type="Unknown";;
esac
# replaces -DLINUX
if test "$zmap_os_type" = "Linux" ; then
   AC_MSG_RESULT([Got OS type of LINUX])
   AC_DEFINE(LINUX, [], [Operating System is LINUX.  See zmapThreads/zmapThreads_P.h])
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

#AC_PROG_LIBTOOL
#AC_DISABLE_SHARED
AM_INIT_AUTOMAKE

AC_LANG(C)
AC_PATH_X

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
#PKG_CHECK_MODULES(GTK, $GTK2_MODULES,,)
PKG_CHECK_MODULES(GTK, $GTK2_MODULES,,)

# now foocanvas
# really we want to allow foocanvas and foocanvas-patched(our version)
# to live side by side but for now lets leave it like this
PKG_CHECK_MODULES(FOOCANVAS, libfoocanvas = 0.1,,AC_MSG_ERROR(foocanvas is required))

# needs curl, we'll be looking using curl-config
# use ac_path_lib (http://ac-archive.sourceforge.net/rleigh/ac_path_lib.m4)
# stop looking with pkg-config
AC_PATH_LIB_LIBCONFIG
AC_PATH_LIB(curl, 7.13.0 -nocheck, [], [], [], AC_MSG_RESULT(Found ${CURL_LIBS}), AC_MSG_ERROR(curl-config not found.  please install or alter \$PATH = $PATH))

# AceConn & md5
# Apologies for the monkeying around, but without a nice method for finding
# the aceconn library, this is the best I could do.  It's kinda sieve like
# default to use the library
aceconn=false
aceconn_location=
# might possibly have a version not requiring this.
AC_ARG_WITH(ace-conn,
           AS_HELP_STRING(--with-ace-conn,
                          need to specify this to set up AceConn library),
           aceconn=true;aceconn_location=$withval,)

ACECONN_CFLAGS=
ACECONN_LIBS="-lAceConn -lmd5"

if $aceconn ; then
   if test "x$aceconn_location" = "xyes"
     then
     echo 
   elif test "x$aceconn_location" = "xno"; then
      AC_MSG_ERROR(... We currently need -lAceConn.  You cannot specify --without-ace-conn)
   else
      ACECONN_LIBS="-L$aceconn_location $ACECONN_LIBS"
      ACECONN_CFLAGS="-I$aceconn_location/.."
   fi
else
   FOUND_LIBS=
   save_ifs=$IFS; IFS=':'
   for n in $LD_LIBRARY_PATH; do
      if [ test -e $n/libAceConn.a ]; then
         AC_MSG_NOTICE(Found libAceConn.a in $n)
            FOUND_LIBS="-L/usr/lib -L$n"
      fi
   done
   IFS=$save_ifs
   ACECONN_LIBS="$FOUND_LIBS $ACECONN_LIBS"
fi

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
zmapWindow"
AC_SUBST(ZMAP_SRC_DIRS)


AC_CONFIG_FILES([Makefile
zmapApp/Makefile
zmapConfig/Makefile
zmapControl/Makefile
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
include/Makefile
lib/Makefile
zmap.pc
])

AC_OUTPUT
