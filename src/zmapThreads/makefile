#
# Makes the manager part of the ZMap application.
#


# local macros
PUB_HDRS = zmapSys.h zmapConn.h zmapServer.h
APP_PRIV_HDRS = zmapConn_P.h zmapSlave_P.h
APP_SRC = zmapConn.c zmapSlave.c zmapConnutils.c
APP_OBJ = zmapConn.o zmapSlave.o zmapConnutils.o

# the main routines will need some different treatment maybe...???? try putting
# them all together, don't know if it will work.....

#
# These must all be set for the common includes to work.
#
CURRENT_LIB = $(ZMAPTHR_LIB)
CURRENT_OBJ = $(APP_OBJ)
CURRENT_DEP = $(PUB_HDRS) $(APP_PRIV_HDRS)
CURRENT_SRC = $(APP_PRIV_HDRS) $(APP_SRC)

#
# point to common make include file which contains all the rules etc.
#
ROOT_DIRECTORY = ..
MAKE_DIR = $(ROOT_DIRECTORY)/zmapMake
include $(MAKE_DIR)/build.make
