#!/bin/echo dot script please source

# nothing here.

#FOOCANVAS_DEV=$ZMAP_TRUE
#FOOCANVAS_PREFIX=/nfs/team71/acedb/edgrif/CHECKOUT/foocanvas_test/linux

# Required to get backtraces otherwise many symbols are not passed to ELF linker.
CFLAGS_args="$CFLAGS_args -rdynamic"

TOTALVIEW_PREFIX=/usr/toolworks/totalview.8.3.0-0

# If there's a need to debug code that forks then this will be needed
# LDFLAGS="-L$TOTALVIEW_PREFIX/linux-x86/lib -ldbfork"

# For testing alternative versions of gtk+
GTK_PREFIX=/software/acedb/gtk
# GTK_PREFIX=/software/acedb/gtk+-2.14.7

# Make sure we look in the right place of autoconf macros...
ACLOCAL_FLAGS="-I $GTK_PREFIX/share/aclocal"

# autotools are also installed in here!
PATH=$GTK_PREFIX/bin:$PATH
