#
# Makes the manager part of the ZMap application.
#


# local macros
WIND_PRIV_HDRS = zmapWindow_P.h
PUB_HDRS = zmapWindow.h
WIND_SRC = zmapWindow.c zmapWindowButtons.c zmapWindowFrame.c zmapWindowmenubar.c
WIND_OBJ = zmapWindow.o zmapWindowButtons.o zmapWindowFrame.o zmapWindowmenubar.o

# the main routines will need some different treatment maybe...???? try putting
# them all together, don't know if it will work.....

#
# These must all be set for the common includes to work.
#
CURRENT_LIB = $(ZMAP_LIB)
CURRENT_OBJ = $(WIND_OBJ)
CURRENT_DEP = $(PUB_HDRS) $(WIND_PRIV_HDRS)
CURRENT_SRC = $(WIND_PRIV_HDRS) $(WIND_SRC)

#
# point to common make include file which contains all the rules etc.
#
ROOT_DIRECTORY = ..
MAKE_DIR = $(ROOT_DIRECTORY)/zmapMake
include $(MAKE_DIR)/build.make
