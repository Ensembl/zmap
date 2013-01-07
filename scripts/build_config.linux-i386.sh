#!/bin/echo dot script please source


# Required to get backtraces otherwise many symbols are not passed to ELF linker.
CFLAGS_args="$CFLAGS_args -rdynamic"


# Set this prefix to target different installations of gtk.
#GTK_PREFIX=/software/acedb/gtk


# This shouldn't get overwritten, note we need /software/acedb in the path
# for libs like foocanvas, AceConn etc.
PKG_CONFIG=$GTK_PREFIX/bin/pkg-config
#PKG_CONFIG_PATH=$GTK_PREFIX/lib/pkgconfig:/software/acedb/lib/pkgconfig
PKG_CONFIG_PATH=$GTK_PREFIX/lib/pkgconfig

# Make sure we look in the right place of autoconf macros...
ACLOCAL_FLAGS="-I $GTK_PREFIX/share/aclocal"

# set up autotools path
PATH=$GTK_PREFIX/bin:$PATH
