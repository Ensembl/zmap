#
# Makes the manager part of the ZMap application.
#


# local macros
SERV_PRIV_HDRS = zmapServer_P.h
PUB_HDRS = zmapServer.h
SERV_SRC = zmapServer.c
SERV_OBJ = zmapServer.o



#
# These must all be set for the common includes to work.
# Need to include AceConn header.
#
CURRENT_LIB = $(ZMAP_LIB)
CURRENT_OBJ = $(SERV_OBJ)
CURRENT_DEP = $(PUB_HDRS) $(SERV_PRIV_HDRS)
CURRENT_SRC = $(SERV_PRIV_HDRS) $(SERV_SRC)
CURRENT_INC = -I$(HOME)/acedb/CODE/Ace-Conn

#
# point to common make include file which contains all the rules etc.
#
ROOT_DIRECTORY = ..
MAKE_DIR = $(ROOT_DIRECTORY)/zmapMake
include $(MAKE_DIR)/build.make

