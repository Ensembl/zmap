## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = gnu

SUBDIRS = include src test doc

EXTRA_DIST = AceConn-1.1.pc.in


pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = AceConn-1.1.pc

$(pkgconfig_DATA): config.status
