#!/bin/echo dot script please source

GTK_PREFIX=/usr/local/gtk

LDFLAGS_args="-Xlinker -bind_at_load"

PATH=$GTK_PREFIX/bin:$PATH
ACLOCAL_FLAGS="-I $GTK_PREFIX/share/aclocal"
PKG_CONFIG=$GTK_PREFIX/bin/pkg-config

UNIVERSAL_BUILD=yes
