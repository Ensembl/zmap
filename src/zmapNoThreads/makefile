#
# Makes the manager part of the ZMap application.
#


#
# point to common make include file which contains all the rules etc.
#
ROOT_DIRECTORY = ..
MAKE_DIR = $(ROOT_DIRECTORY)/zmapMake


# local macros
PUB_HDRS = zmapSys.h zmapConn.h
APP_PRIV_HDRS = zmapConnNoThr_P.h
APP_SRC = zmapConnNoThr.c
APP_OBJ = zmapConnNoThr.o

# the main routines will need some different treatment maybe...???? try putting
# them all together, don't know if it will work.....

#
# These must all be set for the common includes to work.
#
CURRENT_LIB = $(ZMAPNOTHR_LIB)
CURRENT_OBJ = $(APP_OBJ)
CURRENT_DEP = $(PUB_HDRS) $(APP_PRIV_HDRS)
CURRENT_SRC = $(APP_PRIV_HDRS) $(APP_SRC)

#
# include all common defines
#
include $(MAKE_DIR)/common.defs


#
# override any of the common defines that you need to here...
#


#
# include all common targets
#
include $(MAKE_DIR)/common.targets


#
# include your own targets here.
#
