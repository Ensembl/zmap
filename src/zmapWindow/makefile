#
# Makes the Window part of a "view" within a ZMap.
#
 
#
# point to common make include file which contains all the rules etc.
#
ROOT_DIRECTORY = ..
MAKE_DIR = $(ROOT_DIRECTORY)/zmapMake


# local macros
WIND_PRIV_HDRS = zmapWindow_P.h

PUB_HDRS = zmapDraw.h zmapWindow.h zmapWindowDrawFeatures.h

WIND_SRC = zmapWindow.c zmapWindowFrame.c GFF_scaffold.c zmapWindowDrawFeatures.c \
           zmapWindowList.c zmapWindowUtils.c zmapWindowAlignment.c

WIND_OBJ = zmapWindow.o zmapWindowFrame.o GFF_scaffold.o zmapWindowDrawFeatures.o \
           zmapWindowList.o zmapWindowUtils.o zmapWindowAlignment.o

#
# These must all be set for the common includes to work.
#
CURRENT_LIB = $(ZMAP_LIB)
CURRENT_OBJ = $(WIND_OBJ)
CURRENT_DEP = $(PUB_HDRS) $(WIND_PRIV_HDRS)
CURRENT_SRC = $(WIND_PRIV_HDRS) $(WIND_SRC)


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
