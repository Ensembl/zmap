#
# Makes the manager part of the ZMap application.
#


CURL_INC = `curl-config --cflags`

# local macros
SERV_PRIV_HDRS = dasServer_P.h saxparse.h saxparse_P.h
PUB_HDRS = 
SERV_SRC = dasServer.c saxparse.c
SERV_OBJ = dasServer.o saxparse.o

#
# These must all be set for the common includes to work.
# Need to include AceConn header.
#
CURRENT_LIB = $(ZMAPTHR_LIB)
CURRENT_OBJ = $(SERV_OBJ)
CURRENT_DEP = $(PUB_HDRS) $(SERV_PRIV_HDRS)
CURRENT_SRC = $(SERV_PRIV_HDRS) $(SERV_SRC)
CURRENT_INC = -I$(HOME)/acedb/CODE/Ace-Conn $(CURL_INC)

#
# point to common make include file which contains all the rules etc.
#
ROOT_DIRECTORY = ../../
MAKE_DIR = $(ROOT_DIRECTORY)/zmapMake
include $(MAKE_DIR)/build.make

