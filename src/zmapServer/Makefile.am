
include $(top_srcdir)/Makefile.zmap

SUBDIRS = acedb das pipe file

if USE_ENSEMBL
SUBDIRS += ensembl
endif

noinst_LTLIBRARIES = libZMapServer.la libZMapServers.la

libZMapServer_la_SOURCES = zmapServer.h \
zmapServer.c \
zmapServer_P.h \
zmapServerProtocolHandler.c \
zmapServerPrototype.h \
zmapDataSource_P.h \
zmapDataSource.h \
zmapDataSource.c \
$(NULL)

libZMapServer_la_LIBADD  = 
libZMapServer_la_CFLAGS  = $(INCLUDE_COMMON)
libZMapServer_la_LDFLAGS = 

libZMapServers_la_SOURCES = 
libZMapServers_la_LIBADD  = \
acedb/libAcedbServer.la \
pipe/libPipeServer.la   \
file/libFileServer.la   \
das/libDasServer.la     \
$(NULL)

if USE_ENSEMBL
libZMapServers_la_LIBADD += ensembl/libEnsemblServer.la
endif

libZMapServers_la_CFLAGS  = 
libZMapServers_la_LDFLAGS = 

# Extra files to remove for the maintainer-clean target.
#
MAINTAINERCLEANFILES = $(top_srcdir)/zmapServer/Makefile.in




