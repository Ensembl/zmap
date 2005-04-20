#!/bin/sh

set -x

GTK_CFLAGS="-pthread -I/usr/local/gtk2/include/pango-1.0 -I/usr/local/gtk2/include/glib-2.0 -I/usr/local/gtk2/lib/glib-2.0/include -I/usr/local/gtk2/include/gtk-2.0 -I/usr/local/gtk2/lib/gtk-2.0/include -I/usr/local/gtk2/include/atk-1.0 "
GTK_LIBS="-pthread -L/usr/local/gtk2/lib -lgtk-x11-2.0 -lgdk-x11-2.0 -latk-1.0 -lgdk_pixbuf-2.0 -lpangox-1.0 -lpango-1.0 -lgobject-2.0 -lgmodule-2.0 -lgthread-2.0 -lrt -lglib-2.0 -lintl -liconv -lm  "


cc -g -I. \
-I/nfs/team71/analysis/rds/workspace/ZMap/src/include \
$GTK_CFLAGS \
-I/usr/X11R6/include \
-std -fprm d -ieee \
test.c -o test \
$GTK_LIBS \
-L/usr/local/lib -L/usr/X11R6/lib -lX11 zmapXRemote.o 
