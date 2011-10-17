#!/bin/echo dot script please source


# For testing alternative versions of gtk+
GTK_PREFIX=/software/acedb/gtk


# This shouldn't get overwritten.
PKG_CONFIG=$GTK_PREFIX/bin/pkg-config


#PACKAGE_CFG_PATH="$PACKAGE_CFG_PATH:/usr/share/pkgconfig"


# I think I missed a trick installing the autotools
# aclocal doesn't add the system path as well as
# its install path to the search dirs
ACLOCAL_FLAGS="-I $GTK_PREFIX/share/aclocal"


# autotools in /usr is newer.
#PATH=$GTK_PREFIX/bin:$PATH
PATH=$PATH:$GTK_PREFIX/bin

