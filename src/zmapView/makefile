#
# Makes the code that creates/manages/destroys an individual ZMapView
#


# bootstrap stuff, without these make can't find the files to include which define
# all the macros/rules and dependencies.
#
ROOT_DIRECTORY = ..

# local macros
CONT_PRIV_HDRS = zmapView_P.h
PUB_HDRS = zmapView.h
CONT_SRC = zmapView.c
CONT_OBJ = zmapView.o



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
# point to common make include file which contains all the rules etc.
#
ROOT_DIRECTORY = ..
MAKE_DIR = $(ROOT_DIRECTORY)/zmapMake
include $(MAKE_DIR)/build.make

