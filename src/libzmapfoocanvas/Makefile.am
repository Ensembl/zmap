
include $(top_srcdir)/Makefile.common

SUBDIRS = .

noinst_LTLIBRARIES = libzmapfoocanvas.la

libzmapfoocanvasincludedir = $(top_srcdir)/libzmapfoocanvas

libzmapfoocanvas_la_SOURCES =			\
	$(libfoocanvasinclude_HEADERS)		\
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
	libfoocanvastypes.c                     \
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
	libfoocanvas.h                          \
	$(NULL)

libzmapfoocanvas_la_CFLAGS = $(INCLUDE_COMMON)

libzmapfoocanvas_la_marshal = foo-canvas-marshal.list

libzmapfoocanvas_la_DEPENDENCIES = $(nodist_libzmapfoocanvas_la_SOURCES)

libzmapfoocanvas_la_extra_sources = foo-canvas-marshal.h \
foo-canvas-marshal.c \
$(NULL)

nodist_libzmapfoocanvas_la_SOURCES = $(libzmapfoocanvas_la_extra_sources)

BUILT_SOURCES = $(libzmapfoocanvas_la_extra_sources)
CLEANFILES = $(BUILT_SOURCES)
EXTRA_DIST = $(libzmapfoocanvas_la_marshal)   


GENMARSHAL_COMMAND = $(GLIB_GENMARSHAL) --prefix=foo_canvas_marshal


# Extra files to remove for the maintainer-clean target.
#
MAINTAINERCLEANFILES = $(top_srcdir)/libzmapfoocanvas/Makefile.in



.list.h:
	$(GENMARSHAL_COMMAND) --header $< >$@

.list.c:
	$(GENMARSHAL_COMMAND) --body $< >$@



