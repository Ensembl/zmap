#
# - Copy into your own subdirectory.
# - Fill in your own macros, header, objects below.
# - You should not need to alter the final section that includes
#   the common makefile stuff.
#
#


#
# point to common make include file which contains all the rules etc.
#
ROOT_DIRECTORY = ..
MAKE_DIR = $(ROOT_DIRECTORY)/zmapMake


# local macros
PRIV_HDRS = 
PUB_HDRS = zmapFeature.h
PRIV_SRC = zmapFeature.c zmapFeatureUtils.c
PRIV_OBJ = zmapFeature.o zmapFeatureUtils.o
PRIV_LIB = 
#PRIV_INC = -Iany_hdrs_I_need                                                              


#
# These must all be set for the common includes to work.
#
CURRENT_INC = $(PRIV_INC)
CURRENT_LIB = $(ZMAP_LIB)
CURRENT_OBJ = $(PRIV_OBJ)
CURRENT_DEP = $(PUB_HDRS) $(PRIV_HDRS)
CURRENT_SRC = $(PRIV_HDRS) $(PRIV_SRC)


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
