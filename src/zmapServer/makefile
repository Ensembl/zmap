#
# Makes the manager part of the ZMap application.
#


# local macros
SERV_PRIV_HDRS = zmapServer_P.h
PUB_HDRS = zmapServer.h
SERV_SRC = zmapServer.c
SERV_OBJ = zmapServer.o

ALL_SERVER_DIRS = acedb das

# These need to go ....
#ACEDB_PRIV_HDRS = acedbServer_P.h
#ACEDB_SRC = acedbServer.c
#ACEDB_OBJ = acedbServer.o


#
# These must all be set for the common includes to work.
# Need to include AceConn header.
#
CURRENT_LIB = $(ZMAPTHR_LIB)
CURRENT_OBJ = $(SERV_OBJ)
CURRENT_DEP = build_servers $(PUB_HDRS) $(SERV_PRIV_HDRS)
CURRENT_SRC = $(SERV_PRIV_HDRS) $(SERV_SRC)
#CURRENT_INC =

#
# point to common make include file which contains all the rules etc.
#
ROOT_DIRECTORY = ..
MAKE_DIR = $(ROOT_DIRECTORY)/zmapMake
include $(MAKE_DIR)/build.make


build_servers: forcemake
	@for d in $(ALL_SERVER_DIRS); do \
	(cd $$d ; echo ; echo 'make all in '$$d ; $(MAKE) 'all' ; echo) ; \
	done
