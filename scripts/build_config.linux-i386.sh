#!/bin/echo dot script please source


# For some reason aceconn isn't installed in GTK_PREFIX
# This should change asap. Production foocanvas is!
ACECONN_PREFIX=~zmap/prefix/LINUX

# This shouldn't get overwritten.
PKG_CONFIG=$GTK_PREFIX/bin/pkg-config

# autotools are also installed in here!
PATH=$ACECONN_PREFIX/bin:$PATH
