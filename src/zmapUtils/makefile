#
# Makes the manager part of the ZMap application.
#


# local macros
UTILS_SRC = zmapGUIutils.c
UTILS_OBJ = zmapGUIutils.o

# the main routines will need some different treatment maybe...???? try putting
# them all together, don't know if it will work.....

#
# These must all be set for the common includes to work.
#
CURRENT_LIB = $(ZMAP_LIB)
CURRENT_OBJ = $(UTILS_OBJ)
CURRENT_DEP = 
CURRENT_SRC = $(UTILS_SRC)

#
# point to common make include file which contains all the rules etc.
#
ROOT_DIRECTORY = ..
MAKE_DIR = $(ROOT_DIRECTORY)/zmapMake
include $(MAKE_DIR)/build.make
