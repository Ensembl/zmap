#
# Makes the manager part of the ZMap application.
#


# local macros
SERV_PRIV_HDRS = acedbServer_P.h
PUB_HDRS = 
SERV_SRC = acedbServer.c
SERV_OBJ = acedbServer.o

#
# These must all be set for the common includes to work.
# Need to include AceConn header.
#
CURRENT_LIB = $(ZMAPTHR_LIB)
CURRENT_OBJ = $(SERV_OBJ)
CURRENT_DEP = $(PUB_HDRS) $(SERV_PRIV_HDRS)
CURRENT_SRC = $(SERV_PRIV_HDRS) $(SERV_SRC)
CURRENT_INC = -I.. -I$(HOME)/acedb/CODE/Ace-Conn

#
# point to common make include file which contains all the rules etc.
#
ROOT_DIRECTORY = ../../
MAKE_DIR = $(ROOT_DIRECTORY)/zmapMake
include $(MAKE_DIR)/build.make

