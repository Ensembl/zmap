#!/bin/echo dot-script source me, don't run me

####################
# User Configuration
PREFIX=/usr/local

LEAVE_PREVIOUS_BUILD="yes"
CLEAN_BUILD_DIR="no"
CLEAN_DIST_DIR="no"
GET_ONLY="no"

SLEEP=15
SILENT_CD=yes

##########################
# folder & files locations
DIST_DIR=tars
BUILD_DIR=src
# patches is in cvs!
PATCH_DIR=patches
# these two are .cvsignore'd so watch out if changing
BUILD_STATUS_FILE=$BASE_DIR/build_status.sh
BUILD_EXECUTE_CONFIG=$BASE_DIR/build.exe.config.sh


##########################
# Common Configure options
# don't include --prefix=
CONFIGURE_OPTS="--disable-static \
--disable-gtk-doc \
--enable-shared \
--disable-scrollkeeper \
--disable-dependency-tracking \
"

# list of packages to build
BUILD_LIST_OF_PACKAGES="pkg_config \
libtool libpng libjpeg libtiff gettext \
glib atk freetype fontconfig cairo \
pango gtk gnome_common foocanvas"

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
# libtool - libjpeg needs this
# gettext - glib depends on this
# gnome-common - foocanvas needs this

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
PACKAGE_foocanvas_URL=http://cvs.gnome.org
PACKAGE_foocanvas_NAME="libfoocanvas"
PACKAGE_foocanvas_VERSION=20071109
PACKAGE_foocanvas_EXT=tar.gz
PACKAGE_foocanvas_CONFIGURE_OPTS=

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

# fontconfig
PACKAGE_fontconfig_URL=http://fontconfig.org/release/
PACKAGE_fontconfig_NAME="fontconfig"
PACKAGE_fontconfig_VERSION=2.4.92
PACKAGE_fontconfig_EXT=tar.gz
PACKAGE_fontconfig_CONFIGURE_OPTS=

# gettext
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

# libjpeg
PACKAGE_libjpeg_URL=ftp://ftp.gtk.org/pub/gtk/v2.10/dependencies
PACKAGE_libjpeg_NAME=jpegsrc
PACKAGE_libjpeg_VERSION=v6b
PACKAGE_libjpeg_EXT=tar.gz
PACKAGE_libjpeg_CONFIGURE_OPTS=

# libpng
PACKAGE_libpng_URL=ftp://ftp.simplesystems.org/pub/libpng/png/src
PACKAGE_libpng_NAME=libpng
PACKAGE_libpng_VERSION=1.2.23
PACKAGE_libpng_EXT=tar.gz
PACKAGE_libpng_CONFIGURE_OPTS=

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

# pango
PACKAGE_pango_URL=ftp://ftp.gtk.org/pub/pango/1.12
PACKAGE_pango_NAME=pango
PACKAGE_pango_VERSION=1.12.4
PACKAGE_pango_EXT=tar.bz2
PACKAGE_pango_CONFIGURE_OPTS=
PACKAGE_pango_POSTCONFIGURE="eval perl -pi~ -e 's|SUBDIRS = pango modules examples docs tools tests|SUBDIRS = pango modules docs tools tests|g' Makefile && perl -pi~ -e 's|harfbuzz_dump_LDADD = |harfbuzz_dump_LDADD = -Xlinker -framework -Xlinker CoreServices -Xlinker -framework -Xlinker ApplicationServices|g' pango/opentype/Makefile"

# gtk
PACKAGE_gtk_URL=ftp://ftp.gtk.org/pub/gtk/v2.10
PACKAGE_gtk_NAME="gtk+"
PACKAGE_gtk_VERSION=2.10.14
PACKAGE_gtk_EXT=tar.gz
PACKAGE_gtk_CONFIGURE_OPTS=


###################
# program locations
# Some systems are very different when it comes to
# locations of programs.  Hopefully these should help!
ECHO=echo
WGET=wget
RM=rm
GTAR=tar
MKDIR=mkdir
RMDIR=rmdir
# ./configure sometimes uses echo -n and always uses /bin/sh. I've
# found that the -n option isn't recognised in sh on Leopard.
CONFIGURE="/bin/bash ./configure"
MAKE=make
# in case you need to sudo make install
MAKE_INSTALL="$MAKE install"
PKG_CONFIG=pkg-config
PATCH=patch


# scripts can be setup to alter the above variables so
# build_save_execution_config can save the current state of the
# variables.  We reload the file here if it exists.  This is
# especially useful for use in patch scripts which don't copy across
# the environment without exporting _all_ of the variables in this
# file...

if [ -f $BUILD_EXECUTE_CONFIG ]; then
    . $BUILD_EXECUTE_CONFIG
fi
