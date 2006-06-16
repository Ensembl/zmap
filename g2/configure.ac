
AC_PREREQ(2.59)
dnl Some package information...
AC_INIT([g2], [0.70] ,[g2@g2.sourceforge.net])

AC_CONFIG_SRCDIR([src/g2_device.c])
AC_CONFIG_HEADER([src/config.h])

dnl libtool version-info http://www.gnu.org/software/libtool/manual.html#Updating-version-info
G2_CURRENT=0
G2_REVISION=7
G2_AGE=0
AC_SUBST(G2_CURRENT)
AC_SUBST(G2_REVISION)
AC_SUBST(G2_AGE)

dnl major, minor and revision numbers
G2LIB_MAJOR=0
G2LIB_MINOR=7
G2LIB_REVISION=0


echo "************************************************************"
echo "* Welcome to ./configure for $PACKAGE_STRING"
echo "* Cmd: $0 $@"
echo "* Environment:"
echo "*   LDFLAGS: $LDFLAGS"
echo "*   CFLAGS : $CFLAGS"
echo "*   CC     : $CC"
echo "* continuing..."
echo "*"

#
# check C compiler, preprocesor, etc.
#
AC_PROG_CC
AC_PROG_CPP
AC_PROG_CXX

AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_PROG_RANLIB

#
# check cannonical system name,
# and set options for shared libraries (black magic)
dnl AC_CANONICAL_BUILD
case "$host" in
     *cygwin*)
        echo "* Host matched CYGWIN"   
       	LDFLAGS="$LDFLAGS -L/lib/w32api"
        AC_CHECK_LIB(gdi32, main,
          [have_win32='true';
           LDFLAGS="$LDFLAGS -lgdi32";
           AC_DEFINE(DO_WIN32, [], [ Define to compile Win32 code. ])])
        AC_CHECK_LIB(bgd, main, 
          [have_bgd=yes], 
          [have_bgd=no])
                ;;
     *)
        echo "* Host matched Default"
        AC_DEFINE(UNIX, [], [Operating system is UNIX]);;
esac


AC_CHECK_LIB(dl, dlopen, have_dl=yes, have_dl=no)

AM_INIT_AUTOMAKE

dnl
dnl Try to locate the X Window System include files and libraries
dnl and add /usr/local to include and lib path and add -lm (for testing)
dnl
AC_PATH_XTRA
CFLAGS="$CFLAGS $X_CFLAGS -I/usr/local/include"
LDFLAGS="$LDFLAGS $X_LIBS -L/usr/local/lib"

dnl
dnl Check supported devices.
dnl
dnl The window stuff from the host case statement needs a Conditional
dnl after the AM_INIT_AUTOMAKE
AM_CONDITIONAL(WIN32, test x$have_win32 = xtrue)

AC_CHECK_LIB(m, main,
  [have_ps='true';
   have_fig='true';
   LDFLAGS="$LDFLAGS -lm";
   AC_DEFINE(DO_PS,  [], [ Define to compile postscript. ]) 
   AC_DEFINE(DO_FIG, [], [ Define to compile FIG. ])],
   [break])
AM_CONDITIONAL(PS, test x$have_ps = xtrue)
AM_CONDITIONAL(FIG, test x$have_fig = xtrue)

AC_CHECK_LIB(X11, main, 
  [have_x11='true';
   LDFLAGS="$LDFLAGS -lX11";
   AC_DEFINE(DO_X11, [], [ Define to compile X11. ])])
AM_CONDITIONAL(X11, test x$have_x11 = xtrue)

AC_CHECK_LIB(gd, main, 
  [have_gd='true'], 
  [have_gd='false';
   AC_MSG_WARN([libgd could not be found - see http://www.boutell.com])])

if test "x$have_gd" = xtrue
then
        have_gd='false';
	LDFLAGS="$LDFLAGS -liconv -lgd";
	AC_CHECK_FUNC(gdImagePng, 
          [AC_CHECK_HEADERS(gd.h, 
             [have_gd='true'
              AC_DEFINE(DO_GD, [], [ Define to compile GD ])], 
             [have_gd='false'])])
fi
AM_CONDITIONAL(GD, test x$have_gd = xtrue)
   
#
# Declare variables which we want substituted in the Makefile.in's and config.h.in
#
AC_SUBST(DO_PS)
AC_SUBST(DO_FIG)
AC_SUBST(DO_X11)
AC_SUBST(DO_WIN32)
AC_SUBST(DO_GD)

AC_HEADER_STDC
AC_CHECK_HEADERS(limits.h)



#
# finally create makefiles using Makefile.in
#
echo
echo "* Tests complete... Creating output..."
echo
AC_CONFIG_FILES(
  [Makefile 
   src/Makefile 
   demo/Makefile 
   doc/Makefile 
   g2_perl/Makefile.PL])
AC_OUTPUT

echo
echo "************************************************************"
echo "* Configuration Summary for $PACKAGE_STRING:"
echo "* ----------------------------------"
echo "* Environment:"
echo "*   LDFLAGS: $LDFLAGS"
echo "*   CFLAGS : $CFLAGS"
echo "*   CC     : $CC"
echo "*"
echo "* Supported devices:"

if test "x$have_ps"     = "xtrue"; then echo "*    PostScript"; fi
if test "x$have_fig"    = "xtrue"; then echo "*    FIG"; fi
if test "x$have_x11"    = "xtrue"; then echo "*    X11"; fi
if test "x$have_win32"  = "xtrue"; then echo "*    Win32"; fi
if test "x$have_gd"     = "xtrue"; then echo "*    GD"; fi

echo "*"
echo "* Unsupported devices:"

if test "x$have_ps"    != "xtrue"; then echo "*    PostScript"; fi
if test "x$have_fig"   != "xtrue"; then echo "*    FIG"; fi
if test "x$have_x11"   != "xtrue"; then echo "*    X11"; fi
if test "x$have_win32" != "xtrue"; then echo "*    Win32"; fi
if test "x$have_gd"    != "xtrue"; then echo "*    GD"; fi

echo "*"
echo "************************************************************"
