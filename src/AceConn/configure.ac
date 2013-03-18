#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.61])
AC_INIT([AceConn], [1.1.0], [acedb@sanger.ac.uk], [aceconn], [http://www.acedb.org/Software/Downloads/extras.shtml])

AC_CONFIG_SRCDIR([include/AceConn.h])
AC_CONFIG_HEADERS([config.h])

AC_CONFIG_MACRO_DIR([m4])

# acedb server assumes 4 byte ints so client code must as well.
AC_STRICT_COMPILE_CHECK_SIZEOF(int, 4)

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB([readline], [readline],,AC_MSG_ERROR(AceConn test program simpleclient requires gnu readline library.))

# AceConn needs glib
GLIB_MODULES="glib-2.0"
PKG_CHECK_MODULES([GLIB], [${GLIB_MODULES}])
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h netdb.h netinet/in.h stdlib.h string.h sys/ioctl.h sys/socket.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_CHECK_FUNCS([gethostbyname memset select socket])


AC_CONFIG_FILES([AceConn-1.1.pc
                 Makefile
                 doc/Makefile
                 include/Makefile
                 src/Makefile
                 test/Makefile])

dnl Disable shared libs so AceConn will be bound in.
AC_DISABLE_SHARED

AM_PROG_LIBTOOL
AM_INIT_AUTOMAKE


AC_OUTPUT
