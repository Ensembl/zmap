
include $(top_srcdir)/Makefile.common

SUBDIRS = .

noinst_LTLIBRARIES = libCanvasDemo.la

libCanvasDemo_la_SOURCES = containers.c\
	containers.h \
	gui.c \
	gui.h \
	utils.c \
	utils.h \
	$(NULL)
libCanvasDemo_la_LDFLAGS = 
libCanvasDemo_la_CFLAGS  = $(INCLUDE_COMMON)

bin_PROGRAMS = crcanvas_demo goocanvas_demo goocanvas_mv_demo foocanvas_demo long_item_test

##################################################

crcanvas_demo_SOURCES      = crcanvas_demo.c crcanvas_demo.h
crcanvas_demo_CPPFLAGS     = $(AM_CPPFLAGS) $(DEMO_CANVAS_CFLAGS)
crcanvas_demo_LDFLAGS      = 
crcanvas_demo_LDADD        = $(DEMO_CANVAS_LIBS) -lCanvasDemo
crcanvas_demo_DEPENDENCIES = $(noinst_LTLIBRARIES)

##################################################

goocanvas_demo_SOURCES      = goocanvas_demo.c goocanvas_demo.h
goocanvas_demo_CPPFLAGS     = $(AM_CPPFLAGS) $(DEMO_CANVAS_CFLAGS)
goocanvas_demo_LDFLAGS      = 
goocanvas_demo_LDADD        = $(crcanvas_demo_LDADD)
goocanvas_demo_DEPENDENCIES = $(noinst_LTLIBRARIES)

##################################################

goocanvas_mv_demo_SOURCES      = goocanvas_mv_demo.c goocanvas_mv_demo.h
goocanvas_mv_demo_CPPFLAGS     = $(AM_CPPFLAGS) $(DEMO_CANVAS_CFLAGS)
goocanvas_mv_demo_LDFLAGS      = 
goocanvas_mv_demo_LDADD        = $(crcanvas_demo_LDADD)
goocanvas_mv_demo_DEPENDENCIES = $(noinst_LTLIBRARIES)

##################################################

foocanvas_demo_SOURCES      = foocanvas_demo.c foocanvas_demo.h
foocanvas_demo_CPPFLAGS     = $(FOOCANVAS_CFLAGS) $(DEMO_CANVAS_CFLAGS)
foocanvas_demo_LDFLAGS      = 
foocanvas_demo_LDADD        = $(crcanvas_demo_LDADD)
foocanvas_demo_DEPENDENCIES = $(noinst_LTLIBRARIES)

##################################################

long_item_test_SOURCES      = long_item_test.c long_item_test.h
long_item_test_CPPFLAGS     = $(DEMO_CANVAS_CFLAGS)
long_item_test_LDFLAGS      = $(FOOCANVAS_LIBS) $(DEMO_CANVAS_LIBS)
long_item_test_LDADD        = $(crcanvas_demo_LDADD)
long_item_test_DEPENDENCIES = $(noinst_LTLIBRARIES)

