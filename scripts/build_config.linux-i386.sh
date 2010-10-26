#!/bin/echo dot script please source


#echo "RUNNING build_config.linux-i386.sh !!"


# Required to get backtraces otherwise many symbols are not passed to ELF linker.
CFLAGS_args="$CFLAGS_args -rdynamic"


# Set this prefix to target different installations of gtk.
# (With an updated Debian "lenny" distribution we should be ok with /usr)
#GTK_PREFIX=/software/acedb/gtk
GTK_PREFIX=/usr

# This shouldn't get overwritten, note we need /software/acedb in the path
# for libs like foocanvas, AceConn etc.
PKG_CONFIG=$GTK_PREFIX/bin/pkg-config
PKG_CONFIG_PATH=$GTK_PREFIX/lib/pkgconfig:/software/acedb/lib/pkgconfig

# Make sure we look in the right place of autoconf macros...
ACLOCAL_FLAGS="-I $GTK_PREFIX/share/aclocal"

# autotools are also installed in here!
PATH=$GTK_PREFIX/bin:$PATH

