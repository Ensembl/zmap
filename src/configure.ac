#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.


# the bootstrap script makes the file zmap_version.m4 and puts zmap's current
# version in it as the macro VERSION_NUMBER which is used in AC_INIT() below.
m4_include([zmap_version.m4])


# Needed for EBI Redhat which is old....the below was added after autoconf 2.63
# copied from: https://github.com/Netflix/dynomite/pull/252/commits/13d63441143aac4aab09e4a67a1ea779cea436f0
#
m4_ifndef([m4_esyscmd_s], [m4_define([m4_chomp_all], [m4_format([[%.*s]], m4_bregexp(m4_translit([[$1]], [/], [/ ]), [/*$]), [$1])])])
m4_ifndef([m4_esyscmd_s], [m4_define([m4_esyscmd_s], [m4_chomp_all(m4_esyscmd([$1]))])])


AC_PREREQ(2.61)
AC_INIT([ZMap], [VERSION_NUMBER], [zmap@sanger.ac.uk], [zmap], [http://www.sanger.ac.uk/resources/software/zmap/])

# do we need this ??
AC_CONFIG_AUX_DIR([config])

# check we can find the source
AC_CONFIG_SRCDIR([zmapApp/zmapAppmain_c.cpp])
AC_CONFIG_HEADERS([config.h])

# Set up local m4 macros
AC_CONFIG_MACRO_DIR([m4])



# Check for programs.
#
#
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET


AC_PATH_PROGS(PERL, perl perl5)

AC_ARG_VAR(GLIB_GENMARSHAL, [The glib-genmarshal executable.])
AC_CHECK_PROG(GLIB_GENMARSHAL, glib-genmarshal, glib-genmarshal)


# Check for C++11 language support
#
AX_CXX_COMPILE_STDCXX_0X()


# External libraries
#
# Note that macros for external libraries define the same symbol as:
#    - an automake variable via AM_CONDITIONAL
#    - a macro available to the compiler via AC_DEFINE
#    - a shell variable via AC_SUBST
#

# Check for openssl
#
AX_CHECK_OPENSSL()

AC_CHECK_HEADERS([opensslv.h], [HAVE_OPENSSL_HEADER=1])
AM_CONDITIONAL([HAVE_SSL], [ test -n "$HAVE_OPENSSL_HEADER" || test -n "$OPENSSL_INCLUDES" ])

AM_COND_IF([HAVE_SSL], 
           [ echo 'openssl is available!' ])
AM_COND_IF([HAVE_SSL], 
           [AC_DEFINE([HAVE_SSL], [1], [Define to 1 if openssl is available])])
AM_COND_IF([HAVE_SSL], 
           [AC_SUBST([HAVE_SSL], [have_ssl])])


# Check for aceconn
#
AM_CONDITIONAL([USE_ACECONN], [ test -f "$srcdir/AceConn/configure.ac" ])
AM_COND_IF([USE_ACECONN],
           [ echo 'Aceconn is defined !!!' ])
AM_COND_IF([USE_ACECONN],
           [AC_DEFINE([USE_ACECONN], [1], [AceConn interface is available.])])
AM_COND_IF([USE_ACECONN],
           [AC_SUBST([USE_ACECONN], [use_aceconn])])






# Check for ensembl
#
# check is more complex:
#  - we need to check whether mysql is available
#  - then we check to see if the developer requested that zmap reference
#    ensembl but we do not build it locally, it's found from LDFLAGS specified by the developer.
#  - then we check to see if the developer wants to build it locally by checking for the makefile.
#
# Note that the below seems arcane and it is, but that's autoconf/automake for you....
#

# Check for mysql
AX_LIB_MYSQL()           # Standard macro to test for mysql
AM_CONDITIONAL([HAVE_MYSQL_LIB], [ test -n "$MYSQL_VERSION" ])
AM_COND_IF([HAVE_MYSQL_LIB], [AC_SUBST([HAVE_MYSQL_LIB], ['mysql lib is available'])])

# Also need to check for headers - need mysql include directory in cflags or AC_CHECK_HEADERS
# fails
AM_COND_IF([HAVE_MYSQL_LIB], [[ CFLAGS="$MYSQL_CFLAGS $CFLAGS" ]])
AC_CHECK_HEADERS([mysql.h], [HAVE_MYSQL_HEADER=1])

AM_CONDITIONAL([HAVE_MYSQL], [ test -n "$HAVE_MYSQL_HEADER" && test -n "$HAVE_MYSQL_LIB" ])
AM_COND_IF([HAVE_MYSQL],
           [AC_SUBST(HAVE_MYSQL, 'mysql is available')])

#AM_COND_IF([HAVE_MYSQL],
#           [ echo 'MYSQL IS AVAILABLE !!!' ])

# If we have mysql then check for develop asking to reference but not build ensembl extension, also
# check whether developer asked for local build of ensembl. Note that autogen.sh makes these
# two options mutually exclusive.
AM_CONDITIONAL([REFERENCE_ENSEMBL], [ test -n "$HAVE_MYSQL" && test -f "$srcdir/ensembl_ref_but_no_build" ])
   
AM_CONDITIONAL([USE_ENSEMBL_SUBDIR], [ test -n "$HAVE_MYSQL" && test -f "$srcdir/ensc-core/src/configure.ac" ])

if test -e "$srcdir/ensc-core/src/configure.ac"; then
AC_CONFIG_SUBDIRS([ensc-core/src])
fi

#AM_COND_IF([REFERENCE_ENSEMBL],
#           [ echo 'ENSEMBL IS AVAILABLE BUT WE ARE ONLY REFERENCING IT !!!' ])

#AM_COND_IF([USE_ENSEMBL_SUBDIR],
#           [ echo 'ENSEMBL IS AVAILABLE AND WE ARE BUILDING IT LOCALLY !!!' ])

# ok....we need a bit of indirection here to create a shell variable we can use in a test
# to set USE_ENSEMBL which must be set for the build.
AM_COND_IF([REFERENCE_ENSEMBL],
           [AC_SUBST(NEED_ENSEMBL, 'need ensembl')])

AM_COND_IF([USE_ENSEMBL_SUBDIR],
           [AC_SUBST(NEED_ENSEMBL, 'need ensembl')])

AM_CONDITIONAL([USE_ENSEMBL], [ test -n "$NEED_ENSEMBL" ])

#AM_COND_IF([USE_ENSEMBL],
#           [ echo 'USE_ENSEMBL IS DEFINED !!!' ])

AM_COND_IF([USE_ENSEMBL],
           [AC_DEFINE([USE_ENSEMBL], [1], [ 'Ensembl interface is available' ])])
AM_COND_IF([USE_ENSEMBL],
           [AC_SUBST(USE_ENSEMBL, 'Ensembl interface is available')])

# For local builds copy the ensc-core code to the build directory.
AM_COND_IF([USE_ENSEMBL_SUBDIR],
           [AC_DEFINE([USE_ENSEMBL_SUBDIR], [1], [Ensembl interface is built as a subdirectory of ZMap.])])
AM_COND_IF([USE_ENSEMBL_SUBDIR],
           [AC_SUBST([USE_ENSEMBL_SUBDIR], [use_ensembl_subdir])])
AM_COND_IF([USE_ENSEMBL_SUBDIR],
           [ cp -r $srcdir/ensc-core . ])


# Check for htslib
#
AM_CONDITIONAL([USE_HTSLIB], [ test -f "$srcdir/htslib/configure.ac" ])
AM_COND_IF([USE_HTSLIB],
           [AC_DEFINE([USE_HTSLIB], [1], [Htslib interface is available.])])
AM_COND_IF([USE_HTSLIB],
           [AC_SUBST([USE_HTSLIB], [use_htslib])])
AM_COND_IF([USE_HTSLIB],
           [ cp -r $srcdir/htslib . ])

# Check for zeromq
#
AM_CONDITIONAL([USE_ZEROMQ], [ test -f "$srcdir/zeromq/configure" ])
AM_COND_IF([USE_ZEROMQ],
           [AC_DEFINE([USE_ZEROMQ], [1], [Zeromq interface is available.])])
AM_COND_IF([USE_ZEROMQ],
           [AC_SUBST([USE_ZEROMQ], [use_zeromq])])


# Set ZMap version number
#
# Check if this is a Git repository and, if so, use output
# of git describe abbreviated to just tag which for us is the version string.
# otherwise use PACKAGE_VERSION which will include info about the current
# status in the version info (gets set from VERSION_NUMBER in zmap_version.m4).
# 
if test -d "$srcdir/../.git" 
then AC_DEFINE_UNQUOTED([ZMAP_VERSION_NUMBER], ["`git log --format='%cs:%h' -n 1 HEAD`"], [ZMap package version])
else AC_DEFINE([ZMAP_VERSION_NUMBER], [PACKAGE_VERSION], [ZMap package version])
fi



echo \
"---------------------------------------------------------------------
 Welcome to ./configure for $PACKAGE_STRING
 Cmd: $0 $@
 Environment:
   CXX              : $CXX
   CXXFLAGS         : $CXXFLAGS
   CC               : $CC
   CFLAGS           : $CFLAGS
   LD               : $LD
   LDFLAGS          : $LDFLAGS
   PATH             : $PATH
   PACKAGE_CFG      : $PKG_CONFIG
   PACKAGE_CFG_PATH : $PKG_CONFIG_PATH
   LD_LIBRARY_PATH  : $LD_LIBRARY_PATH
 continuing...
"
echo "---------------------------------------------------------------------
"



# what are we building on?
AC_CANONICAL_BUILD
case $build in
     *-*-linux*)
     zmap_os_type="Linux";;
     *-*-darwin*)
     zmap_os_type="darwin" ;;
     *)
     zmap_os_type="Unknown";;
esac

# replaces -DLINUX
if test "$zmap_os_type" = "Linux" ; then
   AC_MSG_RESULT([Got OS type of LINUX])
   AC_DEFINE(LINUX, [], [Operating System is LINUX.  See zmapThreadSource/zmapThreads_P.h])

elif test "$zmap_os_type" = "darwin" ; then

   AC_MSG_RESULT([Got OS type of Darwin])
   AC_DEFINE(DARWIN, [], [Operating System is DARWIN.  See zmapThreadSource/zmapThreads_P.h])

fi



ZMAP_FALSE=no
ZMAP_TRUE=yes

AC_ARG_WITH([macosx-sdk-path],
        AS_HELP_STRING([--with-maxosx-sdk-path=path], [enable building of mac osx universal binary using 'path' to SDK]),
        [], [with_macosx_sdk_path=$ZMAP_FALSE])

# NEEDS REMOVING.....
#
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


dnl Disable shared libs so foocanvas is bound in to zmap.
AC_DISABLE_SHARED
AC_ENABLE_STATIC



AC_PROG_CXXCPP
AC_PROG_CXX

AM_PROG_CC_C_O

LT_INIT
LT_LANG([C++])

# make options
# tar-ustar - use the ustar tar format as this allows file paths longer than 99 chars.
# subdir-objects - compile code that is in a subdirectory of the directory containing the Makefile.am
#                  (don't set this....there's a bug in autmake 1.nn if you do !)
#AM_INIT_AUTOMAKE([tar-ustar] [subdir-objects])
AM_INIT_AUTOMAKE([tar-ustar])



# The check libraries are NOT installed by default on most peoples linux
# machines therefore the configure script included in our autoconf dist file
# for zmap will nearly always fail. Therefore I've commented out this test
# until we can resolve this in a better way. Anyone installing zmap will not
# be affected unless they try "make test" which they should do anyway as we
# don't currently support any test programs.
#
# PKG_CHECK_MODULES([CHECK], [check >= 0.9.4])


AC_ARG_ENABLE(debug,
        AS_HELP_STRING(--enable-debug,enable debug info compilation (no)),
        deb=$enableval, deb=no)

AC_ARG_ENABLE(profile,
        AS_HELP_STRING(--enable-profile,enable profiling debug info compilation (no)),
        prof=$enableval, prof=no)


AC_SUBST(PKG_CONFIG)
AC_SUBST(CFLAGS)
AC_SUBST(CPPFLAGS)
AC_SUBST(LDFLAGS)


# set up the zmap version, used for naming dist tar file etc etc.
#VERSION=ZMAP_VERSION_NUMBER
#AC_SUBST(VERSION)



################################################################################
################################################################################
# Checks for libraries.
################################################################################
################################################################################

# gtk first

GTK2_MODULES="pango >= 0.1 gtk+-2.0 >= 2.10.0 glib-2.0 gthread-2.0"
PKG_CHECK_MODULES([GTK], [${GTK2_MODULES}])



# canvas_choice compilation is disabled for now, it should be moved
# out of the src dir into its own dir. It does not fit in with
# doing a dist build.
#
#CANVAS_TESTING=""
#GTK2_MODULES="gtk+-2.0 >= 2.10.0"
#PKG_CHECK_MODULES([LATESTGTK], [${GTK2_MODULES}], [
#  CANVAS_TESTING="canvas_choice"
#  AC_MSG_RESULT([Found GTK 2.10])
#], [
#  CANVAS_TESTING=""
#  AC_MSG_RESULT([GTK 2.10 not found.])
#])
#
#DEMO_CANVAS_MODULES="crcanvas >= 0.12 goocanvas >= 0.7 geocanvas >= 1.0.7"
#PKG_CHECK_MODULES([DEMO_CANVAS], [${DEMO_CANVAS_MODULES}], [
#  AC_MSG_RESULT([Found Canvases])
#], [
#  if test "x$CANVAS_TESTING" != "x"; then
#    CANVAS_TESTING=""
#    AC_MSG_RESULT([Are you sure?])
#  else
#    AC_MSG_RESULT([Canvases not found, but neither was GTK 2.10])
#  fi
#])
#
#AC_SUBST([CANVAS_TESTING])


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
#PKG_CHECK_MODULES([FOOCANVAS], [libzmapfoocanvas = 0.1],,AC_MSG_ERROR(zmap version of foocanvas is required))


# needs curl, we'll be looking using curl-config
# LIBCURL_CHECK_CONFIG ([DEFAULT-ACTION], [MINIMUM-VERSION],
#                       [ACTION-IF-YES], [ACTION-IF-NO])
LIBCURL_CHECK_CONFIG([],7.11.0,[AC_MSG_RESULT(LIBCURL FLAGS ${LIBCURL})],[AC_MSG_ERROR(Sorry we need at least version 7.11.0)])


#
# Remove aceconn from here....
#
#PKG_CHECK_MODULES([ACECONN], [AceConn-1.1],,AC_MSG_ERROR(AceConn is required))

#CPPFLAGS="$CPPFLAGS $ACECONN_CFLAGS"
#LDFLAGS="$LDFLAGS $ACECONN_LIBS"

#AC_CHECK_LIB(AceConn, [main],
#  [ have_ace_conn='true' ],
#  [ have_ace_conn='false';
#    AC_MSG_ERROR(AceConn package is required) ])


# and substitute values....
# Should be able to use @VARIABLE@ in Makefile.am
#AC_SUBST(FOOCANVAS_CFLAGS)
#AC_SUBST(FOOCANVAS_LIBS)
#AC_SUBST(ACECONN_CFLAGS)
#AC_SUBST(ACECONN_LIBS)
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

AC_CHECK_HEADERS([fcntl.h inttypes.h libintl.h locale.h memory.h stdint.h stdlib.h string.h strings.h sys/param.h sys/socket.h termios.h unistd.h utime.h])


################################################################################
################################################################################
# Checks for typedefs, structures, and compiler characteristics.
################################################################################
################################################################################
AC_C_CONST
AC_TYPE_MODE_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T

################################################################################
################################################################################
# Checks for library functions.
################################################################################
################################################################################
AC_FUNC_FORK
AC_FUNC_STAT

AC_CHECK_FUNCS([floor gethostname memchr memmove memset mkdir setlocale sqrt strcasecmp strncasecmp strstr strtol uname])

#LDFLAGS="$LDFLAGS $FOOCANVAS_LIBS $ACECONN_LIBS"
#LDFLAGS="$LDFLAGS $ACECONN_LIBS"

#if test "x$with_macosx_sdk_path" == "x$ZMAP_FALSE" ; then
#  AC_CHECK_LIB(zmapfoocanvas, foo_canvas_zmap,:, AC_MSG_ERROR(no patched libfoocanvas!))
# HAVE_LIBFOOCANVAS is now defined
#fi

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
zmapAppServices \
zmapConfig \
zmapControl \
zmapDataSource \
zmapDraw \
zmapFeature \
zmapGFF \
zmapRemoteControl \
zmapRemoteControl/remotetest \
zmapServer \
zmapThreadSource \
zmapThreadsLib \
zmapUtils \
zmapView \
zmapWindow \
zmapXML"



AC_SUBST(ZMAP_SRC_DIRS)



AC_CONFIG_FILES([Makefile
zmapApp/Makefile
zmapAppServices/Makefile
zmapConfig/Makefile
zmapControl/Makefile
zmapDataSource/Makefile
zmapDraw/Makefile
zmapFeature/Makefile
zmapGFF/Makefile
zmapRemoteControl/Makefile
zmapRemoteControl/remotetest/Makefile
zmapServer/Makefile
zmapServer/acedb/Makefile
zmapServer/datastream/Makefile
zmapServer/ensembl/Makefile
zmapServer/file/Makefile
zmapServer/pipe/Makefile
zmapThreadSource/Makefile
zmapThreadsLib/Makefile
zmapUtils/Makefile
zmapView/Makefile
zmapWindow/Makefile
zmapXML/Makefile
include/Makefile
libzmapfoocanvas/Makefile
zmap.pc
dotZMap:dotZmap.in
])




# Run autoconf in the aceconn directory.
#
AM_COND_IF([USE_ACECONN],
           [AC_CONFIG_SUBDIRS([AceConn])])



# Build the gbtools library, if it exists. (This can be optionally
# copied into the zmap tree and we build it. Otherwise, the user
# must make sure it is installed locally.)
#
if test -e "$srcdir/gbtools/configure.ac"; then
AC_CONFIG_SUBDIRS([gbtools])
fi

AM_CONDITIONAL([USE_GBTOOLS], [test -e "$srcdir/gbtools/configure.ac"])


# Run autoconf in the htslib directory.
#
AM_COND_IF([USE_HTSLIB],
           [AC_CONFIG_SUBDIRS([htslib])])



# Build the zeromq package, complicated because we need to tell it to build the
# static library as well as the dynamic. I got this recipe from:
#         https://github.com/tracebox/tracebox/blob/master/configure.ac
#
ac_configure_args_pre="$ac_configure_args"

on_cygwin="no"

# OS-specific tests
case "${host_os}" in
    *cygwin*)
        ac_configure_args_post="$ac_configure_args"
        on_cygwin="yes"
        ;;
    *)
        ac_configure_args_post="$ac_configure_args --enable-static"
        ;;
esac
ac_configure_args="$ac_configure_args_post"

AM_CONDITIONAL(ON_CYGWIN, test "x$on_cygwin" = "xyes")

AC_CONFIG_COMMANDS_PRE([ac_configure_args="$ac_configure_args_pre"])
AC_CONFIG_COMMANDS_POST([ac_configure_args="$ac_configure_args_post"])

AM_COND_IF([USE_ZEROMQ],
           [AC_CONFIG_SUBDIRS([zeromq])])



# Finish up.
#
AC_OUTPUT


echo
echo "             srcdir = $srcdir"
echo "      ac_top_srcdir = $ac_top_srcdir"
echo "ac_top_build_prefix = $ac_top_build_prefix"
echo "          ac_srcdir = $ac_srcdir"
echo

echo \
"---------------------------------------------------------------------

 ${PACKAGE_NAME} Version ${PACKAGE_VERSION}

       Prefix: '${prefix}'
 C++ Compiler: '${CXX} ${CXXFLAGS} ${CPPFLAGS}'
   C Compiler: '${CC} ${CFLAGS} ${CPPFLAGS}'

 Now type 'make @<:@<target>@:>@'
   where the optional @<:@<target>@:>@ is:
     all       - build all binaries (default)
     install   - build and install everything
     <program> - build specific program
"

if test -z "$USE_ACECONN"
then
  echo " Warning: AceConn is not installed so Acedb server access is disabled."
  echo
fi

if test -z "$HAVE_MYSQL"
then
  echo " Warning: MySql is not installed so ensc-core access is disabled."
  echo
fi

if test -z "$HAVE_SSL"
then
  echo " Warning: openssl is not installed. ZMap will not be able to load"
  echo " files from https locations (including via Track Hub)."
  echo
fi

if test -z "$USE_ENSEMBL"
then
  echo " Warning: ensc-core is not installed so Ensembl server access is disabled."
  echo
fi

if test -z "$USE_HTSLIB"
then
  echo " Warning: htslib is not installed so BAM/CRAM/VCF handling is disabled."
  echo
fi

if test -z "$USE_ZEROMQ"
then
  echo " Warning: zeromq is not installed so ZMap RemoteControl is disabled."
  echo
fi

echo "---------------------------------------------------------------------
"



# debug.....
#echo "====== debugging......"
#echo "CC = $CC"
#echo "CFLAGS = $CFLAGS"
#
#echo "CXX = $CXX"
#echo "CXXFLAGS = $CXXFLAGS"
#
#echo "LDFLAGS = $LDFLAGS"
#

