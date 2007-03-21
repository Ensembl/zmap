
include $(top_srcdir)/Makefile.common


noinst_LIBRARIES = libCanvasDemo.a

libCanvasDemo_a_SOURCES = containers.c\
	containers.h \
	gui.c \
	gui.h \
	utils.c \
	utils.h \
	$(NULL)

bin_PROGRAMS = crcanvas_demo goocanvas_demo goocanvas_mv_demo foocanvas_demo long_item_test

##################################################

crcanvas_demo_SOURCES = crcanvas_demo.c \
	crcanvas_demo.h

crcanvas_demo_CPPFLAGS = @DEMO_CANVAS_CFLAGS@

crcanvas_demo_LDFLAGS = -L. \
	@GTK_LIBS@ \
	@DEMO_CANVAS_LIBS@ \
	$(NULL)

crcanvas_demo_LDADD = -lCanvasDemo

crcanvas_demo_DEPENDENCIES = $(noinst_LIBRARIES)

##################################################

goocanvas_demo_SOURCES = goocanvas_demo.c \
	goocanvas_demo.h

goocanvas_demo_CPPFLAGS = @DEMO_CANVAS_CFLAGS@

goocanvas_demo_LDFLAGS = -L. \
	@GTK_LIBS@ \
	@DEMO_CANVAS_LIBS@ \
	$(NULL)

goocanvas_demo_LDADD = -lCanvasDemo

goocanvas_demo_DEPENDENCIES = $(noinst_LIBRARIES)

##################################################

goocanvas_mv_demo_SOURCES = goocanvas_mv_demo.c \
	goocanvas_mv_demo.h

goocanvas_mv_demo_CPPFLAGS = @DEMO_CANVAS_CFLAGS@

goocanvas_mv_demo_LDFLAGS = -L. \
	@GTK_LIBS@ \
	@DEMO_CANVAS_LIBS@ \
	$(NULL)

goocanvas_mv_demo_LDADD = -lCanvasDemo

goocanvas_mv_demo_DEPENDENCIES = $(noinst_LIBRARIES)

##################################################

foocanvas_demo_SOURCES = foocanvas_demo.c \
	foocanvas_demo.h

foocanvas_demo_CPPFLAGS = @FOOCANVAS_CFLAGS@\
	@DEMO_CANVAS_CFLAGS@

foocanvas_demo_LDFLAGS = -L. \
	@FOOCANVAS_LIBS@
	@GTK_LIBS@ \
	@DEMO_CANVAS_LIBS@ \
	$(NULL)

foocanvas_demo_LDADD = -lCanvasDemo

foocanvas_demo_DEPENDENCIES = $(noinst_LIBRARIES)

##################################################

long_item_test_SOURCES = long_item_test.c \
	long_item_test.h

long_item_test_CPPFLAGS = @DEMO_CANVAS_CFLAGS@ 

long_item_test_LDFLAGS = -L . \
	@GTK_LIBS@ \
	@DEMO_CANVAS_LIBS@ \
	$(NULL)

long_item_test_LDADD = -lCanvasDemo

long_item_test_DEPENDENCIES = $(noinst_LIBRARIES)
