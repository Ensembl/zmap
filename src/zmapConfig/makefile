#
# Makes the configuration part of the ZMap application.
#


#
# point to common make include file which contains all the rules etc.
#
ROOT_DIRECTORY = ..
MAKE_DIR = $(ROOT_DIRECTORY)/zmapMake


# local macros
PUB_HDRS = zmapConfig.h
CONF_PRIV_HDRS = zmapConfig_P.h
CONF_SRC = zmapConfig.c zmapConfigRead.c zmapConfigGet.c zmapConfigWrite.c zmapConfigUtils.c
CONF_OBJ = zmapConfig.o zmapConfigRead.o zmapConfigGet.o zmapConfigWrite.o zmapConfigUtils.o


#
# These must all be set for the common includes to work.
#
CURRENT_LIB = $(ZMAP_LIB)
CURRENT_OBJ = $(CONF_OBJ)
CURRENT_DEP = $(PUB_HDRS) $(CONF_PRIV_HDRS)
CURRENT_SRC = $(CONF_PRIV_HDRS) $(CONF_SRC)


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
