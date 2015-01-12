
include $(top_srcdir)/Makefile.common

# Only compile ensembl server if we have mysql installed
if USE_MYSQL

SUBDIRS = .

noinst_LTLIBRARIES = libEnsemblServer.la

ENSC_CORE_DIR=/Users/gb10/work/checkout/other/ensc-core/src
ENSC_CORE_CFLAGS=-I$(ENSC_CORE_DIR)/DataModel -I$(ENSC_CORE_DIR)/DataAdaptors -I$(ENSC_CORE_DIR)/Util -I$(ENSC_CORE_DIR)/DataModel/Mapper -I$(ENSC_CORE_DIR)/DataModel/Basic -I$(ENSC_CORE_DIR)/Compara/DataAdaptors -I$(ENSC_CORE_DIR)/Compara/DataModel 
ENSC_CORE_OBJ_DIR=$(ENSC_CORE_DIR)/objectfiles

libEnsemblServer_la_SOURCES = ensemblServer.c ensemblServer_P.h
libEnsemblServer_la_LIBADD  = 
libEnsemblServer_la_CFLAGS  = $(INCLUDE_COMMON) -I$(top_srcdir)/zmapServer $(ENSC_CORE_CFLAGS) $(MYSQL_CFLAGS)
libEnsemblServer_la_LDFLAGS = $(ENSC_CORE_DIR)/libensc.a $(MYSQL_LDFLAGS)


# Extra files to remove for the maintainer-clean target.
#
MAINTAINERCLEANFILES = $(top_srcdir)/zmapServer/ensembl/Makefile.in

endif
