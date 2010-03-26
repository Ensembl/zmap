#!/bin/echo dot script please source


# This shouldn't get overwritten.
PKG_CONFIG=$GTK_PREFIX/bin/pkg-config

# autotools are also installed in here!
PATH=$GTK_PREFIX/bin:$PATH
