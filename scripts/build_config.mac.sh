#!/bin/echo dot script please source

GLIB_GENMARSHAL=/opt/local/bin/glib-genmarshal
export GLIB_GENMARSHAL

GTK_PREFIX=/opt/local

LIBTOOL=glibtool
LIBTOOLIZE=glibtoolize

DX_COND_doc=no

LDFLAGS_args="-Xlinker -bind_at_load"
<<<<<<< HEAD

=======
>>>>>>> develop

PATH=$GTK_PREFIX/bin:$PATH
ACLOCAL_FLAGS="-I $GTK_PREFIX/share/aclocal"
PKG_CONFIG=$GTK_PREFIX/bin/pkg-config

# remove universal build junk.....
UNIVERSAL_BUILD=no


