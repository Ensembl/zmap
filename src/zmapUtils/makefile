#
# Makes the manager part of the ZMap application.
#


# local macros
UTILS_PRIV_HDRS = ZMap_P.h
PUB_HDRS = ZMap.h
UTILS_SRC = zmapUtils.c zmapGUIutils.c zmapLogging.c zmapFileUtils.c
UTILS_OBJ = zmapUtils.o zmapGUIutils.o zmapLogging.o zmapFileUtils.o


#
# These must all be set for the common includes to work.
#
CURRENT_LIB = $(ZMAP_LIB)
CURRENT_OBJ = $(UTILS_OBJ)
CURRENT_DEP = $(PUB_HDRS) $(UTILS_PRIV_HDRS)
CURRENT_SRC = $(UTILS_PRIV_HDRS) $(UTILS_SRC)


#
# point to common make include file which contains all the rules etc.
#
ROOT_DIRECTORY = ..
MAKE_DIR = $(ROOT_DIRECTORY)/zmapMake
include $(MAKE_DIR)/build.make
