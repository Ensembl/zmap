#
# ZMap main makefile
#
# Can be used to build both the ZMap library and the ZMap application.
#

#
# point to common make include file which contains all the rules etc.
#
ROOT_DIRECTORY = .
MAKE_DIR = $(ROOT_DIRECTORY)/zmapMake


#
# include all common defines
#
include $(MAKE_DIR)/common.defs


#
# override any of the common defines that you need to here...
#
APP_DIR = zmapApp
MAKE_DIRS = $(APP_DIR) zmapManager zmapControl zmapView zmapWindow zmapConfig zmapGFF zmapServer zmapThreads zmapUtils zmapNoThreads

DOXY_CONFIG = Doxyfile


all: makebindir libs zmap 
	@echo 'made ZMap libraries and executables...'

clean: makebindir
	@for d in $(MAKE_DIRS); do \
	(cd $$d ; echo ; echo 'make '$@' in '$$d ; $(MAKE) $@ ; echo) ; \
	done
	@rm -f $(ZMAPAPP_LIB) $(ZMAP_LIB) $(ZMAPTHR_LIB) $(ZMAPNOTHR_LIB)

# note that we issue a "make all" in the subdirectories, not a "make libs"
libs: makebindir 
	@for d in $(MAKE_DIRS); do \
	(cd $$d ; echo ; echo 'make all in '$$d ; $(MAKE) 'all' ; echo) ; \
	done

zmap : makebindir
	@cd $(APP_DIR) ; echo ; echo 'make '$@ ; $(MAKE) $@ ; echo

zmapC : makebindir
	@cd $(APP_DIR) ; echo ; echo 'make '$@ ; $(MAKE) $@ ; echo



# Not sure how to frame the dependencies here...needs some thought....
docs :  forcemake
	$(HOME)/tools/doxygen $(DOXY_CONFIG)


# Note that we only include the base targets here as we need special versions of the other
# targets such as "all" for this root makefile.
include $(MAKE_DIR)/common.base_targets


# actually this is all more trouble than its worth really...it would be better
# to have it in a build script really or a totally separate make file...as is
# done in acedb....Think about how to do this more elegantly...
#
# Dependency used by all makes to check that environment variable ZMAP_PLATFORM
# has been set and that the platform defs file actually exists in the make
# defs directory.
#
platform:
	@if [ "$(ZMAP_PLATFORM)" -a -f $(MAKEDEFS_FILE) ] ; \
	then \
	  if test ! -d $(BINDIR) ; \
	  then \
	    echo ; \
	    echo "making "$(BINDIR)" directory" ; \
	    mkdir $(BINDIR) ; \
	  fi \
	else \
	  echo ; \
	  echo 'To compile you must set the environment variable "ZMAP_PLATFORM"' ; \
	  echo 'to one of the following:' ; \
	  echo ; cd $(MAKEDEFS_DIR) ; \
	  for i in `ls *$(MAKEDEFS_SUFFIX)` ; do \
	    echo `basename $$i $(MAKEDEFS_SUFFIX)` ; \
	  done ; \
	  echo ; exit 1 ; \
	fi


