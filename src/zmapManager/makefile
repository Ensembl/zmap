#
# Makes the manager part of the ZMap application.
#


#
# point to common make include file which contains all the rules etc.
#
ROOT_DIRECTORY = ..
MAKE_DIR = $(ROOT_DIRECTORY)/zmapMake


# local macros
PUB_HDRS = zmapConn.h zmapWindow.h zmapManager.h
MAN_PRIV_HDRS = zmapManager_P.h
MAN_SRC = zmapManager.c
MAN_OBJ = zmapManager.o


#
# These must all be set for the common includes to work.
#
CURRENT_LIB = $(ZMAPAPP_LIB)
CURRENT_OBJ = $(MAN_OBJ)
CURRENT_DEP = $(PUB_HDRS) $(MAN_PRIV_HDRS)
CURRENT_SRC = $(MAN_PRIV_HDRS) $(MAN_SRC)



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
