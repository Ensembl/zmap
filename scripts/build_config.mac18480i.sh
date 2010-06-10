#!/bin/echo dot script please source


GLIB_GENMARSHAL=/opt/local/bin/glib-genmarshal
export GLIB_GENMARSHAL

GTK_PREFIX=/opt/local

LIBTOOL=glibtool
LIBTOOLIZE=glibtoolize

LDFLAGS_args="-Xlinker -syslibroot  -Xlinker /opt/local/lib -Xlinker -bind_at_load"

PATH=$GTK_PREFIX/bin:$PATH
ACLOCAL_FLAGS="-I $GTK_PREFIX/share/aclocal"
PKG_CONFIG=$GTK_PREFIX/bin/pkg-config

UNIVERSAL_BUILD=no

