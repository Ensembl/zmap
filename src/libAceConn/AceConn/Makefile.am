## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = gnu

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = include src test doc

EXTRA_DIST = autogen.sh AceConn-1.1.pc.in


pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = AceConn-1.1.pc

$(pkgconfig_DATA): config.status
