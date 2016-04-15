
include $(top_srcdir)/Makefile.common

SUBDIRS = pipe file trackhub

if USE_ACECONN
SUBDIRS += acedb
endif

if USE_ENSEMBL
SUBDIRS += ensembl
endif


noinst_LTLIBRARIES = libZMapServer.la libZMapServers.la

libZMapServer_la_SOURCES = zmapServer.hpp \
zmapServer.cpp \
zmapServer_P.hpp \
zmapServerProtocolHandler.cpp \
zmapServerPrototype.hpp \
zmapDataSource_P.hpp \
zmapDataSource.hpp \
zmapDataSource.cpp \
$(NULL)

libZMapServer_la_LIBADD  = 
libZMapServer_la_CFLAGS  = $(INCLUDE_COMMON)
libZMapServer_la_LDFLAGS = 

libZMapServers_la_SOURCES = 
libZMapServers_la_LIBADD  = \
pipe/libPipeServer.la   \
file/libFileServer.la   \
trackhub/libTrackhubServer.la \
$(NULL)

if USE_ACECONN
libZMapServers_la_LIBADD += acedb/libAcedbServer.la
endif

if USE_ENSEMBL
libZMapServers_la_LIBADD += ensembl/libEnsemblServer.la
endif

libZMapServers_la_CFLAGS  = 
libZMapServers_la_LDFLAGS = 

# Extra files to remove for the maintainer-clean target.
#
MAINTAINERCLEANFILES = $(top_srcdir)/zmapServer/Makefile.in




