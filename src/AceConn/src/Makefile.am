
AM_CPPFLAGS = -I$(top_srcdir)/include @GLIB_CFLAGS@
LDADD = @GLIB_LIBS@

lib_LTLIBRARIES = libAceConn.la

libAceConn_la_SOURCES = aceconn.c aceconnsock.c aceconnutils.c md5c.c

noinst_HEADERS = aceconn_.h global.h md5.h

libAceConn_la_LDFLAGS = -version-info 0:0:0
