
include $(top_srcdir)/Makefile.common

SUBDIRS = acedb das file .

noinst_LTLIBRARIES = libZMapServer.la libZMapServers.la

libZMapServer_la_SOURCES = zmapServer.h \
zmapServer.c \
zmapServer_P.h \
zmapServerProtocolHandler.c \
zmapServerPrototype.h \
$(NULL)
libZMapServer_la_LIBADD  = 
libZMapServer_la_CFLAGS  = $(INCLUDE_COMMON)
libZMapServer_la_LDFLAGS = 

libZMapServers_la_SOURCES = 
libZMapServers_la_LIBADD  = \
acedb/libAcedbServer.la \
file/libFileServer.la   \
pipe/libPipeServer.la   \
das/libDasServer.la     \
$(NULL)
libZMapServers_la_CFLAGS  = 
libZMapServers_la_LDFLAGS = 
