AM_CPPFLAGS = \
	-I$(top_srcdir) 				\
	-I$(top_builddir)				\
	$(WARN_CFLAGS)					\
	$(LIBFOOCANVAS_CFLAGS)			\
        -DFOOCANVASLIBDIR=\""$(libdir)"\" 		\
        -DFOOCANVASDATADIR=\""$(datadir)"\" 		\
        -DFOOCANVASPIXMAPDIR=\""$(datadir)/pixmaps"\"	\
        -DFOOCANVASBINDIR=\""$(bindir)"\" 		\
        -DFOOCANVASLOCALSTATEDIR=\""$(localstatedir)"\" 	\
        -DFOOCANVASLOCALEDIR=\""$(gnomelocaledir)"\" 	\
	-DGTK_VERSION=\""$(GTK_VERSION)"\"		\
	-DG_LOG_DOMAIN=\"FooCanvas\"			\
	-DVERSION=\"$(VERSION)\"

LDADD =	\
	$(top_builddir)/libfoocanvas/libzmapfoocanvas.la \
	$(INTLLIBS)

noinst_PROGRAMS = canvas_demo

canvas_demo_SOURCES =		\
	canvas.c		\
	canvas-primitives.c	\
	canvas-arrowhead.c	\
	canvas-features.c	\
	canvas-fifteen.c	\
	canvas-scalability.c	\
	canvas_demo.h

EXTRA_DIST =			\
	flower.png		\
	toroid.png



