#
# Makes the code that creates/manages/destroys an individual ZMap
#


# use this as a template....

# bootstrap stuff, without these make can't find the files to include which define
# all the macros/rules and dependencies.
#
ROOT_DIRECTORY = ..

# local macros
#ONT_PRIV_HDRS = ZMap_P.h zmapTopWindow_P.h
ONT_PRIV_HDRS = ZMap_P.h
PUB_HDRS = ZMap.h
CONT_SRC = zmapControl.c zmapTopWindow.c zmapTopWindowButtons.c zmapTopWindowFrame.c zmapTopWindowMenubar.c
CONT_OBJ = zmapControl.o zmapTopWindow.o zmapTopWindowButtons.o zmapTopWindowFrame.o zmapTopWindowMenubar.o



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

