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

#PUB_HDRS = seqregion.h stringbucket.h zmapcommon.h zmapcontrol.h zmap.h zmapWindow.h 
PUB_HDRS = zmapWindow.h 

#WIND_SRC = seqregion.c stringbucket.c \
#	zmapbccol.c zmapcalls.c zmapcols.c zmapcontrol.c \
#	zmapsequence.c zmapsplit.c zmapWindow.c zmapWindowFrame.c
WIND_SRC = zmapWindow.c zmapWindowFrame.c GFF_scaffold.c

#WIND_OBJ = seqregion.o stringbucket.o \
#	zmapbccol.o zmapcalls.o zmapcols.o zmapcontrol.o \
#	zmapsequence.o zmapsplit.o zmapWindow.o zmapWindowFrame.o
WIND_OBJ = zmapWindow.o zmapWindowFrame.o GFF_scaffold.o

ZMAP_LIB = libzmap.a

FUNCS = zmapfuncs.c zmapfuncs.h

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
