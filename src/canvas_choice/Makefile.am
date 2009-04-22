
include $(top_srcdir)/Makefile.common


noinst_LTLIBRARIES = libCanvasDemo.la

libCanvasDemo_la_SOURCES = containers.c\
	containers.h \
	gui.c \
	gui.h \
	utils.c \
	utils.h \
	$(NULL)
libCanvasDemo_la_LDFLAGS = -all-static
libCanvasDemo_la_LIBADD  = $(GTK_LIBS)
libCanvasDemo_la_CFLAGS  = $(AM_CPPFLAGS) $(CFLAGS) $(GTK_CFLAGS)

bin_PROGRAMS = crcanvas_demo goocanvas_demo goocanvas_mv_demo foocanvas_demo long_item_test

##################################################

crcanvas_demo_SOURCES      = crcanvas_demo.c crcanvas_demo.h
crcanvas_demo_CPPFLAGS     = $(DEMO_CANVAS_CFLAGS)
crcanvas_demo_LDFLAGS      = 
crcanvas_demo_LINK         = $(CC) $(AM_CFLAGS) $(CFLAGS) $(AM_LDFLAGS) $(LDFLAGS) -o $@
crcanvas_demo_LDADD        = $(GTK_LIBS) $(DEMO_CANVAS_LIBS) -L. -lCanvasDemo 
crcanvas_demo_DEPENDENCIES = $(noinst_LTLIBRARIES)

##################################################

goocanvas_demo_SOURCES      = goocanvas_demo.c goocanvas_demo.h
goocanvas_demo_CPPFLAGS     = $(DEMO_CANVAS_CFLAGS)
goocanvas_demo_LDFLAGS      = $(GTK_LIBS) $(DEMO_CANVAS_LIBS)
goocanvas_demo_LINK         = $(CC) $(AM_CFLAGS) $(CFLAGS) $(AM_LDFLAGS) $(LDFLAGS) -o $@
goocanvas_demo_LDADD        = -L. -lCanvasDemo
goocanvas_demo_DEPENDENCIES = $(noinst_LTLIBRARIES)

##################################################

goocanvas_mv_demo_SOURCES      = goocanvas_mv_demo.c goocanvas_mv_demo.h
goocanvas_mv_demo_CPPFLAGS     = $(DEMO_CANVAS_CFLAGS)
goocanvas_mv_demo_LDFLAGS      = $(GTK_LIBS) $(DEMO_CANVAS_LIBS)
goocanvas_mv_demo_LINK         = $(CC) $(AM_CFLAGS) $(CFLAGS) $(AM_LDFLAGS) $(LDFLAGS) -o $@
goocanvas_mv_demo_LDADD        = -L. -lCanvasDemo
goocanvas_mv_demo_DEPENDENCIES = $(noinst_LTLIBRARIES)

##################################################

foocanvas_demo_SOURCES      = foocanvas_demo.c foocanvas_demo.h
foocanvas_demo_CPPFLAGS     = $(FOOCANVAS_CFLAGS) $(DEMO_CANVAS_CFLAGS)
foocanvas_demo_LDFLAGS      = $(FOOCANVAS_LIBS) $(GTK_LIBS) $(DEMO_CANVAS_LIBS)
foocanvas_demo_LINK         = $(CC) $(AM_CFLAGS) $(CFLAGS) $(AM_LDFLAGS) $(LDFLAGS) -o $@
foocanvas_demo_LDADD        = -L. -lCanvasDemo
foocanvas_demo_DEPENDENCIES = $(noinst_LTLIBRARIES)

##################################################

long_item_test_SOURCES      = long_item_test.c long_item_test.h
long_item_test_CPPFLAGS     = $(DEMO_CANVAS_CFLAGS)
long_item_test_LDFLAGS      = $(FOOCANVAS_LIBS) $(GTK_LIBS) $(DEMO_CANVAS_LIBS)
long_item_test_LDADD        = -L. -lCanvasDemo
long_item_test_LINK         = $(CC) $(AM_CFLAGS) $(CFLAGS) $(AM_LDFLAGS) $(LDFLAGS) -o $@
long_item_test_DEPENDENCIES = $(noinst_LTLIBRARIES)

