bin_PROGRAMS = simpleclient simpletest
simpleclient_SOURCES = simpleclient.c
simpleclient_CPPFLAGS = -I$(top_srcdir)/include @GLIB_CFLAGS@
simpleclient_LDADD =  $(top_builddir)/src/.libs/libAceConn.la -lreadline @GLIB_LIBS@

simpletest_SOURCES = simpletest.c
simpletest_CPPFLAGS = -I$(top_srcdir)/include @GLIB_CFLAGS@
simpletest_LDADD =  $(top_builddir)/src/.libs/libAceConn.la @GLIB_LIBS@

