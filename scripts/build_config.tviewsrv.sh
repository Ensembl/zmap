#!/bin/echo dot script please source

# nothing here.

#FOOCANVAS_DEV=$ZMAP_TRUE
#FOOCANVAS_PREFIX=/var/tmp/rds/software
CFLAGS_args="$CFLAGS_args -rdynamic"

TOTALVIEW_PREFIX=/usr/toolworks/totalview.8.3.0-0

# If there's a need to debug code that forks then this will be needed
# LDFLAGS="-L$TOTALVIEW_PREFIX/linux-x86/lib -ldbfork"

# For testing alternative versions of gtk+
GTK_PREFIX=/software/acedb/gtk
# GTK_PREFIX=/software/acedb/gtk+-2.14.7
