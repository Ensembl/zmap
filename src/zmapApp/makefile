#
# Makes the manager part of the ZMap application.
#


#
# point to common make include file which contains all the rules etc.
#
ROOT_DIRECTORY = ..
MAKE_DIR = $(ROOT_DIRECTORY)/zmapMake


# local macros
APP_PRIV_HDRS = zmapApp_P.h
APP_SRC = zmapAppwindow.c zmapAppmenubar.c zmapAppmanage.c zmapAppconnect.c
APP_OBJ = zmapAppwindow.o zmapAppmenubar.o zmapAppmanage.o zmapAppconnect.o
APP_CC_MAIN_SRC = zmapAppmain_c.c
APP_CC_MAIN_OBJ = zmapAppmain_c.o
APP_CCC_MAIN_OBJ = zmapAppmain_C.o
APP_EXE = zmap zmapC

#
# These must all be set for the common includes to work.
#
CURRENT_LIB = $(ZMAPAPP_LIB)
CURRENT_OBJ = $(APP_OBJ)
CURRENT_DEP = $(PUB_HDRS) $(APP_PRIV_HDRS)
CURRENT_SRC = $(APP_PRIV_HDRS) $(APP_SRC) $(APP_CC_MAIN_SRC)
CURRENT_REMOVE = cd $(BINDIR) ; rm -f $(APP_EXE) $(APP_CC_MAIN_OBJ) $(APP_CCC_MAIN_OBJ)

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


# Make the application programs themselves.

# ZMAP library...
ZMAP_LIB_FLAGS = -L$(BINDIR) -lZMapApp -lZMap -lZMapThr

# Ace-Conn library...This is a bit of hack really...the ACEDB_MACHINE stuff should be
# in a separate subdir as we don't yet know what server we will be contacting....
ACECONN_DIR = $(HOME)/acedb/CODE/Ace-Conn
ACECONN_INCL_DIR = $(ACECONN_DIR)
ACECONN_LIB_DIR = $(ACECONN_DIR)/bin.$(ACEDB_MACHINE)
ACECONN_LIB_FLAGS = -L$(ACECONN_LIB_DIR) -lAceConn -lmd5



zmap: $(APP_CC_MAIN_OBJ) $(ZMAPAPP_LIB) $(ZMAP_LIB) $(ZMAPTHR_LIB)
	$(CC)  -g -pthread  -o $(BINDIR)/$@ $(BINDIR)/zmapAppmain_c.o $(LDFLAGS) $(ALL_LIBS)
	@echo 'zmap (C version) make...'

#	$(CC)  -g -pthread  -o $(BINDIR)/$@ `gtk-config --libs` $(BINDIR)/zmapAppmain_c.o $(ZMAP_LIB_FLAGS) $(ACECONN_LIB_FLAGS) $(GNOME_LIBS) $(GTK_LIBS)
#	@echo 'zmap (C version) make...'

zmapC: $(APP_CCC_MAIN_OBJ) $(ZMAPAPP_LIB) $(ZMAP_LIB) $(ZMAPTHR_LIB)
	$(CCC)  -g -pthread  -o $(BINDIR)/$@ $(BINDIR)/zmapAppmain_c.o $(LDFLAGS) $(ZMAP_LIB_FLAGS) $(ACECONN_LIB_FLAGS) $(GNOME_LIBS) $(GTK_LIBS)
	@echo 'zmap (C++ version) make...'
