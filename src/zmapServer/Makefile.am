
include $(top_srcdir)/Makefile.common

SUBDIRS = datastream file pipe

if USE_ACECONN
SUBDIRS += acedb
endif

if USE_ENSEMBL
SUBDIRS += ensembl
endif


noinst_LTLIBRARIES = libZMapServer.la libZMapServers.la

libZMapServer_la_SOURCES = \
zmapServer.cpp \
zmapServer.hpp \
zmapServerPrototype.hpp \
zmapServerRequestHandler.cpp \
zmapServer_P.hpp \
zmapDataSource_P.hpp \
zmapDataSource.cpp \
$(NULL)

libZMapServer_la_LIBADD  = 
libZMapServer_la_CFLAGS  = $(INCLUDE_COMMON)
libZMapServer_la_LDFLAGS = 


if HAVE_SSL
libZMapServer_la_LDFLAGS += \
$(OPENSSL_LDFLAGS) \
$(OPENSSL_LIBS) \
$(NULL)

libZMapServer_la_CFLAGS += \
$(OPENSSL_INCLUDES) \
$(NULL)
endif


libZMapServers_la_SOURCES = 

libZMapServers_la_LIBADD  = \
datastream/libDataStream.la   \
file/libFileServer.la   \
pipe/libPipeServer.la   \
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




