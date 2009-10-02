#!/bin/echo dot-script source me, don't run me

####################
# User Configuration
PREFIX=/usr/local/gtk+-2.10.universal

LEAVE_PREVIOUS_BUILD="yes"
CLEAN_BUILD_DIR="no"
CLEAN_DIST_DIR="no"
CLEAN_PACKAGE_DIR="no"
GET_ONLY="no"
USE_WGET="no"

SLEEP=15
SILENT_CD=yes

UNIVERSAL_BUILD="no"

##########################
# folder & files locations
DIST_DIR=tars
BUILD_DIR=src
# patches is in cvs!
PATCH_DIR=patches
UNAME_ARCH=$(uname -p)
# these two are .cvsignore'd so watch out if changing
BUILD_STATUS_FILE=$BASE_DIR/build_status.$UNAME_ARCH.sh
BUILD_EXECUTE_CONFIG=$BASE_DIR/build.exe.config.sh

if [ "x$UNIVERSAL_BUILD" == "xyes" ]; then
    export SDK=/Developer/SDKs/MacOSX10.4u.sdk
    export MACOSX_DEPLOYMENT_TARGET=10.4
    export CFLAGS="-isysroot ${SDK} -arch ppc -arch i386 -mmacosx-version-min=10.4"
    export CXXFLAGS="-isysroot ${SDK} -arch ppc -arch i386 -mmacosx-version-min=10.4"

# If we are doing a universal build on a Leopard system we need to tell
# configure to look only in /usr/X11R6 prefix as this is a symlink on Leopard
# but the actual and only location on Tiger! configure expects the reverse!
# We should probably test deployment target ;)
# We use the $SDK/usr/X11R6 to be sure we got the right X11!

# packages requiring this = cairo, pango, gtk, gd, g2
# See their individual blocks of options

    # We add this directory to the path to fix a problem with cups
    # The cups-config in the SDK reports api version of 1.1, quite 
    # a way behind the 1.3 that comes with 10.5. The gtkprintbackendcups.c
    # needed to be patched for the 1.3 version and has ifdefs for 1.2 as
    # well as the now added 1.3. picking up 1.3 from 10.5 makes the 
    # 10.4 ppc compile fail as it hasn't got the 1.3 headers, but has 
    # HAVE_CUPS_API_1_3 defined.
    PATH=$SDK/usr/bin:$PATH
    # I'm unsure on the effect on this yet...

    # might be a good idea to force this...
    #CLEAN_PACKAGE_DIR="yes"
    BUILD_STATUS_FILE=$BASE_DIR/build_status.$MACOSX_DEPLOYMENT_TARGET.sh
fi

##########################
# Common Configure options
# don't include --prefix=
CONFIGURE_OPTS="--disable-static \
--disable-gtk-doc \
--enable-shared \
--disable-scrollkeeper \
--disable-dependency-tracking \
"

# list of packages to build....
# to bring m4 autoconf and automake up-to-date
BUILD_LIST_OF_PACKAGES="m4 autoconf automake"

# everything else
BUILD_LIST_OF_PACKAGES="sed3 sed4 m4 autoconf automake pkg_config expat \
libtool libpng libjpeg libtiff gettext \
glib atk freetype fontconfig cairo \
pango gtk gnome_common foocanvas \
aceconn gd g2 popt"

# or just a single package giving grief
#BUILD_LIST_OF_PACKAGES="m4"

# pkg-config - http://www.freedesktop.org/software/pkgconfig/
# TIFF       - http://www.libtiff.org/
# libpng     - http://www.libpng.org/
# JPEG       - http://www.ijg.org/
# cairo      - http://www.cairographics.org/
# atk        - http://ftp.gnome.org/pub/gnome/sources/atk/
# GLib       - ftp://ftp.gtk.org/pub/glib/2.12
# Pango      - ftp://ftp.gtk.org/pub/pango/1.12

# fontconfig - cairo requirement
# freetype   - cairo requirement

# also ...
# libtool  - libjpeg needs this
# gettext  - glib depends on this
# gnome-common - foocanvas needs this
# expat    - fontconfig/freetype need this
# m4       - not strictly required (autoconf needs it though)
# autoconf - not strictly required (automake needs is though)
# automake - not strictly required (if the version isn't recent enough)
# aceconn  - from the acedb cvs repository
# libgd    - libg2 depends on this
# libg2    - so zmap can print
# popt     - so zmap can parse command line options

# config for packages
# Each package _must_ have 5 variables
# PACKAGE_{}_URL     - url base
# PACKAGE_{}_NAME    - real name
# PACKAGE_{}_VERSION - current version
# PACKAGE_{}_EXT     - dist extension
# PACKAGE_{}_CONFIGURE_OPTS - extra configure options
# When adding more packages keep in mind the gnome/gnu conventions
# the actual file to be downloaded will be
# URL/NAME-VERSION.EXT
# this logic is encoded in the build.sh script

# foocanvas
# Sensitive to rebuild in same directory. Pre script not intelligent enough! 
PACKAGE_foocanvas_URL=file://$BASE_DIR/repos/
PACKAGE_foocanvas_NAME="libfoocanvas"
PACKAGE_foocanvas_VERSION=20080325
PACKAGE_foocanvas_EXT=tar.gz
PACKAGE_foocanvas_CONFIGURE_OPTS=

# AceConn
PACKAGE_aceconn_URL=file://$BASE_DIR/repos/
PACKAGE_aceconn_NAME="libaceconn"
PACKAGE_aceconn_VERSION=0.1
PACKAGE_aceconn_EXT=tar.gz
PACKAGE_aceconn_CONFIGURE_OPTS=

# libg2
PACKAGE_g2_URL=http://kent.dl.sourceforge.net/sourceforge/g2
PACKAGE_g2_NAME="g2"
PACKAGE_g2_VERSION=0.72
PACKAGE_g2_EXT=tar.gz
PACKAGE_g2_CONFIGURE_OPTS=
# If we are doing a universal build on a Leopard system we need to tell
# configure to look only in /usr/X11R6 prefix as this is a symlink on Leopard
# but the actual and only location on Tiger! configure expects the reverse!
# We should probably test deployment target ;)
if [ "x$UNIVERSAL_BUILD" == "xyes" ]; then
  PACKAGE_g2_CONFIGURE_OPTS="--x-includes=$SDK/usr/X11R6/includes --x-libraries=$SDK/usr/X11R6/lib"
fi

# libgd
PACKAGE_gd_URL=http://www.libgd.org/releases
PACKAGE_gd_NAME=gd
PACKAGE_gd_VERSION=2.0.35
PACKAGE_gd_EXT=tar.gz
PACKAGE_gd_CONFIGURE_OPTS=
if [ "x$UNIVERSAL_BUILD" == "xyes" ]; then
  PACKAGE_gd_CONFIGURE_OPTS="--x-includes=/usr/X11R6/includes --x-libraries=/usr/X11R6/lib"
  PACKAGE_gd_CONFIGURE_OPTS="--x-includes=$SDK/usr/X11R6/includes --x-libraries=$SDK/usr/X11R6/lib"
fi

# expat
# You might need to select a different mirror.
# Go to http://sourceforge.net/project/showfiles.php?group_id=10127
# Follow the link to download file and copy link location of the "direct link"
PACKAGE_expat_URL=http://belnet.dl.sourceforge.net/sourceforge/expat
PACKAGE_expat_URL=http://mesh.dl.sourceforge.net/sourceforge/expat
PACKAGE_expat_NAME="expat"
PACKAGE_expat_VERSION=2.0.1
PACKAGE_expat_EXT=tar.gz
PACKAGE_expat_CONFIGURE_OPTS=

# popt
PACKAGE_popt_URL=http://rpm.net.in/mirror/rpm-4.4.x/
PACKAGE_popt_NAME=popt
PACKAGE_popt_VERSION=1.10.4
PACKAGE_popt_EXT=tar.gz
PACKAGE_popt_CONFIGURE_OPTS=

# gtkdoc
PACKAGE_gtkdoc_URL=http://ftp.gnome.org/pub/GNOME/sources/gtk-doc/1.9/
PACKAGE_gtkdoc_NAME="gtk-doc"
PACKAGE_gtkdoc_VERSION=1.9
PACKAGE_gtkdoc_EXT=tar.gz
PACKAGE_gtkdoc_CONFIGURE_OPTS=

# gnome_common
PACKAGE_gnome_common_URL=http://ftp.gnome.org/pub/gnome/sources/gnome-common/2.20/
PACKAGE_gnome_common_NAME=gnome-common
PACKAGE_gnome_common_VERSION=2.20.0
PACKAGE_gnome_common_EXT=tar.gz
PACKAGE_gnome_common_CONFIGURE_OPTS=

# freetype
PACKAGE_freetype_URL=http://download.savannah.gnu.org/releases/freetype/
PACKAGE_freetype_NAME="freetype"
PACKAGE_freetype_VERSION=2.3.5
PACKAGE_freetype_EXT=tar.gz
PACKAGE_freetype_CONFIGURE_OPTS=
if [ "x$UNIVERSAL_BUILD" == "xyes" ]; then
    PACKAGE_freetype_POSTCONFIGURE=build_patch_libtool_dylib
fi

# fontconfig
PACKAGE_fontconfig_URL=http://fontconfig.org/release/
PACKAGE_fontconfig_NAME="fontconfig"
PACKAGE_fontconfig_VERSION=2.4.92
PACKAGE_fontconfig_EXT=tar.gz
PACKAGE_fontconfig_CONFIGURE_OPTS=
if [ "x$UNIVERSAL_BUILD" == "xyes" ]; then
    PACKAGE_fontconfig_POSTCONFIGURE="eval cd fc-arch \
&& make all \
&& cd .. \
&& perl -pi~ -e 's|#define FC_ARCHITECTURE \"x86\"|#ifdef __ppc__\n#define FC_ARCHITECTURE \"ppc\"\n#else\n#define FC_ARCHITECTURE \"x86\"\n#endif|g' fc-arch/fcarch.h"
fi
# gettext
# Sensitive to rebuilding in same directory.
# glib _must_ not be installed before gettext (available in library look ups)
# Something to do with http://bugzilla.gnome.org/show_bug.cgi?id=315437
# duplicate symbol _g_bit_nth_lsf ...
PACKAGE_gettext_URL=ftp://ftp.gnu.org/gnu/gettext
PACKAGE_gettext_NAME="gettext"
PACKAGE_gettext_VERSION=0.17
PACKAGE_gettext_EXT=tar.gz
PACKAGE_gettext_CONFIGURE_OPTS=

# libtool
PACKAGE_libtool_URL=http://ftp.gnu.org/gnu/libtool
PACKAGE_libtool_NAME="libtool"
PACKAGE_libtool_VERSION=1.5.24
PACKAGE_libtool_EXT=tar.gz
PACKAGE_libtool_CONFIGURE_OPTS=

# pkg_config
PACKAGE_pkg_config_URL=http://pkgconfig.freedesktop.org/releases
PACKAGE_pkg_config_NAME="pkg-config"
PACKAGE_pkg_config_VERSION=0.22
PACKAGE_pkg_config_EXT=tar.gz
PACKAGE_pkg_config_CONFIGURE_OPTS=

# glib
PACKAGE_glib_URL=ftp://ftp.gtk.org/pub/glib/2.12
PACKAGE_glib_NAME=glib
PACKAGE_glib_VERSION=2.12.13
PACKAGE_glib_EXT=tar.gz
PACKAGE_glib_CONFIGURE_OPTS=

# libatk
PACKAGE_atk_URL=ftp://ftp.gtk.org/pub/gtk/v2.10/dependencies
PACKAGE_atk_NAME=atk
PACKAGE_atk_VERSION=1.9.1
PACKAGE_atk_EXT=tar.bz2
PACKAGE_atk_CONFIGURE_OPTS=
if [ "x$UNIVERSAL_BUILD" == "xyes" ]; then
    PACKAGE_atk_POSTCONFIGURE=build_patch_libtool_dylib
fi

# libjpeg
# Sensitive to rebuilding in same directory. jconfig.h
PACKAGE_libjpeg_URL=ftp://ftp.gtk.org/pub/gtk/v2.10/dependencies
PACKAGE_libjpeg_NAME=jpegsrc
PACKAGE_libjpeg_VERSION=v6b
PACKAGE_libjpeg_EXT=tar.gz
PACKAGE_libjpeg_CONFIGURE_OPTS=
if [ "x$UNIVERSAL_BUILD" == "xyes" ]; then
    # We need the correct X11 libraries here
    PACKAGE_libjpeg_CONFIGURE_OPTS="--x-includes=$SDK/usr/X11R6/includes --x-libraries=$SDK/usr/X11R6/lib"
fi

# libpng
# 1.2.23 got moved to history, src only contains latest releases
PACKAGE_libpng_URL=ftp://ftp.simplesystems.org/pub/libpng/png/src
PACKAGE_libpng_URL=ftp://ftp.simplesystems.org/pub/libpng/png/src/history
PACKAGE_libpng_NAME=libpng
PACKAGE_libpng_VERSION=1.2.23
PACKAGE_libpng_EXT=tar.gz
PACKAGE_libpng_CONFIGURE_OPTS=
if [ "x$UNIVERSAL_BUILD" == "xyes" ]; then
    PACKAGE_libpng_POSTCONFIGURE=build_patch_libtool_dylib
fi

# libtiff
PACKAGE_libtiff_URL=ftp://ftp.gtk.org/pub/gtk/v2.10/dependencies
PACKAGE_libtiff_NAME=tiff
PACKAGE_libtiff_VERSION=3.7.4
PACKAGE_libtiff_EXT=tar.gz
PACKAGE_libtiff_CONFIGURE_OPTS=

# cairo
PACKAGE_cairo_URL=http://cairographics.org/releases
PACKAGE_cairo_NAME=cairo
# gtk list 1.2.6 as dependency, but it doesn't compile (http://bugs.freedesktop.org/show_bug.cgi?id=10989)
PACKAGE_cairo_VERSION=1.4.8
PACKAGE_cairo_EXT=tar.gz
PACKAGE_cairo_CONFIGURE_OPTS="--enable-shared --disable-quartz --disable-atsui --enable-glitz"
PACKAGE_cairo_CONFIGURE_OPTS="--enable-shared --disable-quartz --disable-atsui --disable-glitz"
if [ "x$UNIVERSAL_BUILD" == "xyes" ]; then
    PACKAGE_cairo_CONFIGURE_OPTS="$PACKAGE_cairo_CONFIGURE_OPTS --x-includes=$SDK/usr/X11R6/includes --x-libraries=$SDK/usr/X11R6/lib"
    export ax_cv_c_float_words_bigendian="no"
    PACKAGE_cairo_POSTCONFIGURE="eval build_patch_libtool_dylib \
&& export ax_cv_c_float_words_bigendian='' \
&& perl -pi~ -e 's|/\* #undef FLOAT_WORDS_BIGENDIAN \*/|#ifdef __ppc__\n#define FLOAT_WORDS_BIGENDIAN 1\n#endif|g;s|/\* #undef WORDS_BIGENDIAN \*/|#ifdef __ppc__\n#define WORDS_BIGENDIAN 1\n#endif|g' config.h && perl -pi~ -e 's|DIST_SUBDIRS = pixman src boilerplate test perf doc|DIST_SUBDIRS = pixman src test perf doc|g;s|am__append_1 = boilerplate test|am__append_1 = test|g' Makefile"

fi

# pango
PACKAGE_pango_URL=ftp://ftp.gtk.org/pub/pango/1.12
PACKAGE_pango_NAME=pango
PACKAGE_pango_VERSION=1.12.4
PACKAGE_pango_EXT=tar.bz2
PACKAGE_pango_CONFIGURE_OPTS=
if [ "x$UNIVERSAL_BUILD" == "xyes" ]; then
    # PACKAGE_pango_CONFIGURE_OPTS="--x-includes=$SDK/usr/X11R6/includes --x-libraries=$SDK/usr/X11R6/lib"
    # Actually we want it configured without X11! 
    PACKAGE_pango_CONFIGURE_OPTS="--without-x"
fi
PACKAGE_pango_POSTCONFIGURE="eval perl -pi~ -e 's|SUBDIRS = pango modules examples docs tools tests|SUBDIRS = pango modules docs tools tests|g' Makefile \
&& perl -pi~ -e 's|harfbuzz_dump_LDADD = |harfbuzz_dump_LDADD = -Xlinker -framework -Xlinker CoreServices -Xlinker -framework -Xlinker ApplicationServices|g' pango/opentype/Makefile"

# gtk
PACKAGE_gtk_URL=ftp://ftp.gtk.org/pub/gtk/v2.10
PACKAGE_gtk_NAME="gtk+"
PACKAGE_gtk_VERSION=2.10.14
PACKAGE_gtk_EXT=tar.bz2
PACKAGE_gtk_CONFIGURE_OPTS=
if [ "x$UNIVERSAL_BUILD" == "xyes" ]; then
  PACKAGE_gtk_CONFIGURE_OPTS="--x-includes=$SDK/usr/X11R6/includes --x-libraries=$SDK/usr/X11R6/lib"
fi

# m4
PACKAGE_m4_URL=http://ftp.gnu.org/gnu/m4/
PACKAGE_m4_NAME="m4"
PACKAGE_m4_VERSION=1.4.9
PACKAGE_m4_EXT=tar.gz
PACKAGE_m4_CONFIGURE_OPTS=
PACKAGE_m4_PRECONFIGURE=m4_pre_configure
# m4 will only build a universal like this. 
# This will possibly break other stuff though. BE WARNED!
# Suggest building m4 on its own first!
function m4_pre_configure
{
    if [ "x$BUILD_LIST_OF_PACKAGES" == "xm4" ]; then
        if [ "x$UNIVERSAL_BUILD" == "xyes" ]; then
            echo "Updating CFLAGS to make universal m4 build succeed..."
	    CFLAGS="$CFLAGS -I/usr/include"
        fi
    fi
}
# N.B. To make the following work you would need to have set
# _both_ variables above! So we now have the preconfigure function.
if [ "x$BUILD_LIST_OF_PACKAGES" == "xm4" ]; then
    if [ "x$UNIVERSAL_BUILD" == "xyes" ]; then
        echo "Updating CFLAGS to make universal m4 build succeed!"
	CFLAGS="$CFLAGS -I/usr/include"
    fi
fi

# autoconf (depends on m4)
PACKAGE_autoconf_URL=http://ftp.gnu.org/gnu/autoconf/
PACKAGE_autoconf_NAME="autoconf"
PACKAGE_autoconf_VERSION=2.61
PACKAGE_autoconf_EXT=tar.gz
PACKAGE_autoconf_CONFIGURE_OPTS=

# automake (depends on autoconf?)
PACKAGE_automake_URL=http://ftp.gnu.org/gnu/automake/
PACKAGE_automake_NAME="automake"
PACKAGE_automake_VERSION=1.9.6
PACKAGE_automake_EXT=tar.gz
PACKAGE_automake_CONFIGURE_OPTS=

# sed 3.02 sed3
PACKAGE_sed3_URL=ftp://ftp.gnu.org/pub/gnu/sed
PACKAGE_sed3_NAME="sed"
PACKAGE_sed3_VERSION=3.02
PACKAGE_sed3_EXT=tar.gz
PACKAGE_sed3_CONFIGURE_OPTS=

# sed 4.1.5 sed4
PACKAGE_sed4_URL=ftp://ftp.gnu.org/pub/gnu/sed
PACKAGE_sed4_NAME="sed"
PACKAGE_sed4_VERSION=4.1.5
PACKAGE_sed4_EXT=tar.gz
PACKAGE_sed4_CONFIGURE_OPTS=

# libcheck
PACKAGE_check_URL=http://kent.dl.sourceforge.net/sourceforge/check
PACKAGE_check_NAME="check"
PACKAGE_check_VERSION=0.9.6
PACKAGE_check_EXT=tar.gz
PACKAGE_check_CONFIGURE_OPTS=


###################
# program locations
# Some systems are very different when it comes to
# locations of programs.  Hopefully these should help!
ECHO=echo
WGET="wget"
CURL="curl"
RM=rm
GTAR=tar
MKDIR=mkdir
RMDIR=rmdir
# ./configure sometimes uses echo -n and always uses /bin/sh. I've
# found that the -n option isn't recognised in sh on Leopard.
CONFIGURE="/bin/bash ./configure"
MAKE=make
if [ "x$(echo $* | grep SUDO=sudo)" != "x" ]; then
    SUDO=sudo
fi
# in case you need to sudo make install
MAKE_INSTALL="$SUDO $MAKE install"
# be careful to make sure jpeg patch also sudo's
PKG_CONFIG=pkg-config
PATCH=patch
PERL=perl

AUTOCONF=autoconf
AUTOMAKE=automake
AUTOHEADER=autoheader
ACLOCAL=aclocal

# scripts can be setup to alter the above variables so
# build_save_execution_config can save the current state of the
# variables.  We reload the file here if it exists.  This is
# especially useful for use in patch scripts which don't copy across
# the environment without exporting _all_ of the variables in this
# file...

if [ -f $BUILD_EXECUTE_CONFIG ]; then
    . $BUILD_EXECUTE_CONFIG
fi
