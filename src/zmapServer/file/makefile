#
# Makes the file accessing version of the server code.
#


#
# point to common make include file which contains all the rules etc.
#
ROOT_DIRECTORY = ../../
MAKE_DIR = $(ROOT_DIRECTORY)/zmapMake


# local macros
SERV_PRIV_HDRS = fileServer_P.h
PUB_HDRS = 
SERV_SRC = fileServer.c
SERV_OBJ = fileServer.o

#
# These must all be set for the common includes to work.
# Need to include AceConn header.
#
CURRENT_LIB = $(ZMAPTHR_LIB)
CURRENT_OBJ = $(SERV_OBJ)
CURRENT_DEP = $(PUB_HDRS) $(SERV_PRIV_HDRS)
CURRENT_SRC = $(SERV_PRIV_HDRS) $(SERV_SRC)
CURRENT_INC = -I..

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

