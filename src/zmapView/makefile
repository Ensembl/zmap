#
# Makes the code that creates/manages/destroys an individual ZMapView
#



#
# point to common make include file which contains all the rules etc.
#
ROOT_DIRECTORY = ..
MAKE_DIR = $(ROOT_DIRECTORY)/zmapMake


# local macros
CONT_PRIV_HDRS = zmapView_P.h
PUB_HDRS = zmapView.h
CONT_SRC = zmapView.c zmapViewUtils.c
CONT_OBJ = zmapView.o zmapViewUtils.o



#
# These must all be set for the common includes to work.
# Need to include AceConn header.
#
CURRENT_LIB = $(ZMAP_LIB)
CURRENT_OBJ = $(CONT_OBJ)
CURRENT_DEP = $(PUB_HDRS) $(CONT_PRIV_HDRS)
CURRENT_SRC = $(CONT_PRIV_HDRS) $(CONT_SRC)
CURRENT_INC = 

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

