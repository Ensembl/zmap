#
# Makes the manager part of the ZMap application.
#


#
# point to common make include file which contains all the rules etc.
#
ROOT_DIRECTORY = ..
MAKE_DIR = $(ROOT_DIRECTORY)/zmapMake


# local macros
SERV_PRIV_HDRS = zmapServer_P.h
PUB_HDRS = zmapServer.h
SERV_SRC = zmapServer.c
SERV_OBJ = zmapServer.o


#
# These must all be set for the common includes to work.
# Need to include AceConn header.
#
CURRENT_LIB = $(ZMAPTHR_LIB)
CURRENT_OBJ = $(SERV_OBJ)
CURRENT_DEP = build_servers $(PUB_HDRS) $(SERV_PRIV_HDRS)
CURRENT_SRC = $(SERV_PRIV_HDRS) $(SERV_SRC)
#CURRENT_INC =

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
ALL_SERVER_DIRS = acedb das

build_servers: forcemake
	@for d in $(ALL_SERVER_DIRS); do \
	echo ; \
	RC=0 ; \
	(cd $$d ; echo 'make all in '$$d ; $(MAKE) 'all') || RC=1 ; \
	if  [ $$RC -eq 1 ] ; \
	then \
	  echo "*** make in $$d failed ***" ; \
	  exit 1 ; \
	fi ; \
	echo ; \
	done

depend: depend_servers

depend_servers: forcemake
	@echo
	@echo "Making $(MAKEDEPEND_FILE) files in subdirectories..."
	@for d in $(ALL_SERVER_DIRS); do \
	(cd $$d ; $(TEST) -e $(MAKEDEPEND_FILE) || touch $(MAKEDEPEND_FILE)) ; \
	(cd $$d ; $(TEST) -w $(MAKEDEPEND_FILE) || chmod u+w $(MAKEDEPEND_FILE)) ; \
	done
	@echo "done..."
	@echo 'Running makedepend in subdirs...'
	@for d in $(ALL_SERVER_DIRS); do \
	echo ; \
	RC=0 ; \
	(cd $$d ; echo 'make depend in '$$d ; $(MAKE) 'depend') || RC=1 ; \
	if  [ $$RC -eq 1 ] ; \
	then \
	  echo "*** make in $$d failed ***" ; \
	  exit 1 ; \
	fi ; \
	echo ; \
	done
	@echo "done..."
