AM_CPPFLAGS = \
	-I$(top_srcdir) 				\
	-I$(top_builddir)				\
	$(WARN_CFLAGS) 					\
	$(LIBFOOCANVAS_CFLAGS)				\
        -DFOOCANVASLIBDIR=\""$(libdir)"\" 		\
        -DFOOCANVASDATADIR=\""$(datadir)"\" 		\
        -DFOOCANVASPIXMAPDIR=\""$(datadir)/pixmaps"\"	\
        -DFOOCANVASBINDIR=\""$(bindir)"\" 		\
        -DFOOCANVASLOCALSTATEDIR=\""$(localstatedir)"\"	\
        -DFOOCANVASLOCALEDIR=\""$(gnomelocaledir)"\" 	\
	-DG_LOG_DOMAIN=\"Foocanvas\"			\
	-DVERSION=\"$(VERSION)\"

lib_LTLIBRARIES = libfoocanvas.la

libfoocanvas_la_LIBADD = \
	$(RENDER_LIBS)				\
	$(LIBFOOCANVAS_LIBS)

libfoocanvasincludedir = $(includedir)/libfoocanvas-1.0/libfoocanvas

libfoocanvasinclude_HEADERS =			\
	foo-canvas-line.h			\
	foozmap-canvas-line-glyph.h		\
	foo-canvas-pixbuf.h			\
	foo-canvas-polygon.h			\
	foo-canvas-rect-ellipse.h		\
	foo-canvas-text.h			\
	foozmap-canvas-text.h			\
	foozmap-canvas-floating-group.h		\
	foo-canvas-util.h			\
	foo-canvas-widget.h			\
	foo-canvas.h				\
	libfoocanvas.h

libfoocanvas_la_SOURCES =			\
	$(libfoocanvasinclude_HEADERS)		\
	foo-canvas-marshal.list			\
	foo-canvas-i18n.h			\
	foo-canvas-line.c			\
	foozmap-canvas-line-glyph.c		\
	foo-canvas-pixbuf.c			\
	foo-canvas-polygon.c			\
	foo-canvas-rect-ellipse.c		\
	foo-canvas-text.c			\
	foozmap-canvas-text.c			\
	foozmap-canvas-floating-group.c		\
	foo-canvas-util.c			\
	foo-canvas-widget.c			\
	foo-canvas.c				\
	libfoocanvastypes.c

GENMARSHAL_COMMAND = $(GLIB_GENMARSHAL) --prefix=foo_canvas_marshal
SUFFIXES = .list

.list.h:
	$(GENMARSHAL_COMMAND) --header $< >$@

.list.c:
	(echo '/* This file has been automatically generated.  Do not edit. */' && \
	echo '#include "$*.h"' && \
	$(GENMARSHAL_COMMAND) --body $< ) >$@

# A hint is needed to build the header first:
BUILT_SOURCES = foo-canvas-marshal.h

# Another hint, see bugs #172211 and #172212:
non-intermediate: foo-canvas-marshal.c

CLEANFILES = foo-canvas-marshal.h foo-canvas-marshal.c

EXTRA_DIST = libfoocanvas.pc.in

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libfoocanvas.pc
